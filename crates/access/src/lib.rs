//! Read-only access to a repository on disk or through the GitHub Contents API.

use std::{
    env, fs,
    path::{Component, Path, PathBuf},
    time::Duration,
};

use serde::{Deserialize, Serialize};
use serde_json::Value;
use ureq::{Agent, http::StatusCode};
use url::Url;

/// Whether a directory entry is a regular file or a directory.
#[derive(Clone, Copy, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum FileType {
    File,
    #[serde(rename = "dir")]
    Directory,
}

/// Metadata returned by [`Accessor::list_dir`].
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct DirEntry {
    pub name: String,
    pub entry_type: FileType,
    /// File size in bytes. Directories have no size.
    pub size: Option<u64>,
}

impl DirEntry {
    pub fn file(name: impl Into<String>, size: Option<u64>) -> Self {
        Self {
            name: name.into(),
            entry_type: FileType::File,
            size,
        }
    }

    pub fn directory(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            entry_type: FileType::Directory,
            size: None,
        }
    }
}

/// Errors produced while accessing a repository.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("invalid path {path:?}: {message}")]
    InvalidPath { path: String, message: String },

    #[error("invalid GitHub repository {repository:?}: {message}")]
    InvalidRepository { repository: String, message: String },

    #[error("filesystem operation failed for {path}: {source}")]
    Io {
        path: PathBuf,
        #[source]
        source: std::io::Error,
    },

    #[error("file is not valid UTF-8: {path}: {source}")]
    Utf8 {
        path: String,
        #[source]
        source: std::string::FromUtf8Error,
    },

    #[error("could not contact GitHub: {0}")]
    Http(#[from] ureq::Error),

    #[error("GitHub denied access to {repository}: {message}")]
    PermissionDenied { repository: String, message: String },

    #[error("GitHub path not found: {0}")]
    NotFound(String),

    #[error("GitHub API request failed ({status}): {message}")]
    GitHub { status: StatusCode, message: String },

    #[error("GitHub returned invalid repository metadata: {0}")]
    Metadata(#[from] serde_json::Error),
}

pub type Result<T> = std::result::Result<T, Error>;

/// A read-only view of a repository.
///
/// Paths are UTF-8, relative to the repository root, and use `/` as their
/// separator. Implementations must not allow a path to escape that root.
pub trait Accessor: Send + Sync {
    /// List one directory, sorted by entry name.
    fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>>;

    /// Read a UTF-8 file, optionally selecting a range measured in Unicode
    /// scalar values (the same semantics as Python text-file offsets).
    fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String>;
}

/// Access files below a directory on the local filesystem.
#[derive(Clone, Debug)]
pub struct DirectoryAccessor {
    directory: PathBuf,
}

impl DirectoryAccessor {
    pub fn new(directory: impl AsRef<Path>) -> Result<Self> {
        let supplied = directory.as_ref();
        let directory = fs::canonicalize(supplied).map_err(|source| Error::Io {
            path: supplied.to_path_buf(),
            source,
        })?;
        if !directory.is_dir() {
            return Err(io_error(
                directory,
                std::io::ErrorKind::NotADirectory,
                "repository root is not a directory",
            ));
        }
        Ok(Self { directory })
    }

    pub fn directory(&self) -> &Path {
        &self.directory
    }

    fn resolve(&self, relative_path: &str) -> Result<PathBuf> {
        let relative = Path::new(relative_path);
        if relative.is_absolute()
            || relative
                .components()
                .any(|component| matches!(component, Component::Prefix(_)))
        {
            return Err(Error::InvalidPath {
                path: relative_path.into(),
                message: "path must be relative and remain inside the repository".into(),
            });
        }

        let unresolved = self.directory.join(relative);
        let resolved = fs::canonicalize(&unresolved).map_err(|source| Error::Io {
            path: unresolved,
            source,
        })?;
        if !resolved.starts_with(&self.directory) {
            return Err(Error::InvalidPath {
                path: relative_path.into(),
                message: "path is outside the accessed directory".into(),
            });
        }
        Ok(resolved)
    }
}

impl Accessor for DirectoryAccessor {
    fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>> {
        let directory = self.resolve(relative_path)?;
        if !directory.is_dir() {
            return Err(io_error(
                directory,
                std::io::ErrorKind::NotADirectory,
                "path is not a directory",
            ));
        }

        let paths = fs::read_dir(&directory).map_err(|source| Error::Io {
            path: directory.clone(),
            source,
        })?;
        let mut result = Vec::new();
        for entry in paths {
            let entry = entry.map_err(|source| Error::Io {
                path: directory.clone(),
                source,
            })?;
            let path = entry.path();

            // Broken links and links whose targets escape the root are not exposed.
            let Ok(resolved) = fs::canonicalize(&path) else {
                continue;
            };
            if !resolved.starts_with(&self.directory) {
                continue;
            }
            let Ok(metadata) = fs::metadata(&resolved) else {
                continue;
            };
            let name = entry.file_name().to_string_lossy().into_owned();
            if metadata.is_dir() {
                result.push(DirEntry::directory(name));
            } else if metadata.is_file() {
                result.push(DirEntry::file(name, Some(metadata.len())));
            }
        }
        result.sort_by(|left, right| left.name.cmp(&right.name));
        Ok(result)
    }

    fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String> {
        let path = self.resolve(relative_path)?;
        if !path.is_file() {
            let kind = if path.is_dir() {
                std::io::ErrorKind::IsADirectory
            } else {
                std::io::ErrorKind::InvalidInput
            };
            return Err(io_error(path, kind, "path is not a regular file"));
        }

        let bytes = fs::read(&path).map_err(|source| Error::Io {
            path: path.clone(),
            source,
        })?;
        let text = String::from_utf8(bytes).map_err(|source| Error::Utf8 {
            path: path.display().to_string(),
            source,
        })?;
        Ok(slice_text(text, offset.unwrap_or(0), limit))
    }
}

fn io_error(path: PathBuf, kind: std::io::ErrorKind, message: &str) -> Error {
    Error::Io {
        path,
        source: std::io::Error::new(kind, message),
    }
}

fn slice_text(text: String, offset: usize, limit: Option<usize>) -> String {
    if offset == 0 && limit.is_none() {
        return text;
    }
    match limit {
        Some(limit) => text.chars().skip(offset).take(limit).collect(),
        None => text.chars().skip(offset).collect(),
    }
}

/// Access a GitHub repository without cloning it.
///
/// Authentication is read from `GITHUB_TOKEN`, then `GH_TOKEN`, when no token
/// is supplied with [`GitHubAccessor::with_token`].
#[derive(Clone, Debug)]
pub struct GitHubAccessor {
    owner: String,
    repository: String,
    git_ref: Option<String>,
    token: Option<String>,
    api_url: Url,
    timeout: Duration,
    client: Agent,
}

fn github_client(timeout: Duration) -> Agent {
    Agent::config_builder()
        .timeout_global(Some(timeout))
        .http_status_as_error(false)
        .build()
        .into()
}

impl GitHubAccessor {
    /// Parse an `owner/repository` pair or a GitHub HTTPS, SSH, or scp-style URL.
    pub fn new(source: impl AsRef<str>) -> Result<Self> {
        let source = source.as_ref();
        let (owner, repository) = parse_repository(source)?;
        Self::from_parts(owner, repository)
    }

    /// Construct from explicit owner and repository path segments.
    pub fn from_parts(owner: impl Into<String>, repository: impl Into<String>) -> Result<Self> {
        let owner = owner.into().trim().to_owned();
        let mut repository = repository.into().trim().to_owned();
        if repository.to_ascii_lowercase().ends_with(".git") {
            repository.truncate(repository.len() - 4);
        }
        validate_segment_pair(&owner, &repository, &format!("{owner}/{repository}"))?;

        Ok(Self {
            owner,
            repository,
            git_ref: None,
            token: env::var("GITHUB_TOKEN")
                .ok()
                .or_else(|| env::var("GH_TOKEN").ok()),
            api_url: Url::parse("https://api.github.com").expect("constant GitHub URL is valid"),
            timeout: Duration::from_secs(30),
            client: github_client(Duration::from_secs(30)),
        })
    }

    pub fn with_ref(mut self, git_ref: impl Into<String>) -> Result<Self> {
        let git_ref = git_ref.into();
        if git_ref.is_empty() {
            return Err(Error::InvalidPath {
                path: git_ref,
                message: "Git ref cannot be empty".into(),
            });
        }
        self.git_ref = Some(git_ref);
        Ok(self)
    }

    pub fn with_token(mut self, token: impl Into<String>) -> Self {
        self.token = Some(token.into());
        self
    }

    pub fn without_token(mut self) -> Self {
        self.token = None;
        self
    }

    /// Override the API root, primarily for GitHub Enterprise or tests.
    pub fn with_api_url(mut self, api_url: impl AsRef<str>) -> Result<Self> {
        let mut parsed = Url::parse(api_url.as_ref()).map_err(|error| Error::InvalidPath {
            path: api_url.as_ref().into(),
            message: format!("invalid API URL: {error}"),
        })?;
        if parsed.cannot_be_a_base() {
            return Err(Error::InvalidPath {
                path: api_url.as_ref().into(),
                message: "API URL cannot be used as a base URL".into(),
            });
        }
        parsed.set_query(None);
        parsed.set_fragment(None);
        self.api_url = parsed;
        Ok(self)
    }

    pub fn with_timeout(mut self, timeout: Duration) -> Result<Self> {
        if timeout.is_zero() {
            return Err(Error::InvalidPath {
                path: String::new(),
                message: "timeout must be positive".into(),
            });
        }
        self.timeout = timeout;
        self.client = github_client(timeout);
        Ok(self)
    }

    pub fn owner(&self) -> &str {
        &self.owner
    }

    pub fn repository(&self) -> &str {
        &self.repository
    }

    pub fn git_ref(&self) -> Option<&str> {
        self.git_ref.as_deref()
    }

    fn contents_url(&self, path: &str) -> Result<Url> {
        let mut url = self.api_url.clone();
        let mut segments = url.path_segments_mut().map_err(|()| Error::InvalidPath {
            path: self.api_url.to_string(),
            message: "API URL cannot contain path segments".into(),
        })?;
        segments.pop_if_empty();
        segments.extend(["repos", &self.owner, &self.repository, "contents"]);
        if !path.is_empty() {
            segments.extend(path.split('/'));
        }
        drop(segments);
        Ok(url)
    }

    fn request(&self, path: &str, accept: &'static str) -> Result<Vec<u8>> {
        let url = self.contents_url(path)?;
        let mut request = self
            .client
            .get(url.as_str())
            .header("accept", accept)
            .header("user-agent", "vibesense")
            .header("x-github-api-version", "2022-11-28");
        if let Some(token) = &self.token {
            request = request.header("authorization", format!("Bearer {token}"));
        }
        if let Some(git_ref) = &self.git_ref {
            request = request.query("ref", git_ref);
        }

        let mut response = request.call()?;
        let status = response.status();
        // The GitHub API permits source files larger than ureq's conservative
        // default body limit, so explicitly read without that default cap.
        let bytes = response
            .body_mut()
            .with_config()
            .limit(u64::MAX)
            .read_to_vec()?;
        if status.is_success() {
            return Ok(bytes);
        }
        let reason = status
            .canonical_reason()
            .unwrap_or("unknown error")
            .to_owned();
        let message = serde_json::from_slice::<Value>(&bytes)
            .ok()
            .and_then(|value| value.get("message")?.as_str().map(str::to_owned))
            .unwrap_or(reason);

        if status == StatusCode::NOT_FOUND {
            return Err(Error::NotFound(if path.is_empty() {
                ".".into()
            } else {
                path.into()
            }));
        }
        if matches!(status, StatusCode::UNAUTHORIZED | StatusCode::FORBIDDEN) {
            return Err(Error::PermissionDenied {
                repository: format!("{}/{}", self.owner, self.repository),
                message,
            });
        }
        Err(Error::GitHub { status, message })
    }

    fn metadata(&self, path: &str) -> Result<Value> {
        let bytes = self.request(path, "application/vnd.github+json")?;
        let value: Value = serde_json::from_slice(&bytes)?;
        if !value.is_array() && !value.is_object() {
            return Err(Error::Metadata(serde_json::Error::io(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                "unexpected repository metadata",
            ))));
        }
        Ok(value)
    }
}

impl Accessor for GitHubAccessor {
    fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>> {
        let path = normalize_github_path(relative_path)?;
        let metadata = self.metadata(&path)?;
        let Some(items) = metadata.as_array() else {
            let kind = metadata.get("type").and_then(Value::as_str);
            let message = if matches!(kind, Some("file" | "symlink" | "submodule")) {
                "path is not a directory"
            } else {
                "GitHub returned unexpected directory metadata"
            };
            return Err(Error::InvalidPath {
                path: display_path(&path).into(),
                message: message.into(),
            });
        };

        let mut result = Vec::new();
        for item in items {
            let Some(name) = item.get("name").and_then(Value::as_str) else {
                continue;
            };
            match item.get("type").and_then(Value::as_str) {
                Some("dir") => result.push(DirEntry::directory(name)),
                Some("file" | "symlink" | "submodule") => {
                    result.push(DirEntry::file(
                        name,
                        item.get("size").and_then(Value::as_u64),
                    ));
                }
                _ => {}
            }
        }
        result.sort_by(|left, right| left.name.cmp(&right.name));
        Ok(result)
    }

    fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String> {
        let path = normalize_github_path(relative_path)?;
        let metadata = self.metadata(&path)?;
        if metadata.is_array() || metadata.get("type").and_then(Value::as_str) == Some("dir") {
            return Err(Error::InvalidPath {
                path: display_path(&path).into(),
                message: "path is a directory".into(),
            });
        }
        if !matches!(
            metadata.get("type").and_then(Value::as_str),
            Some("file" | "symlink" | "submodule")
        ) {
            return Err(Error::InvalidPath {
                path: display_path(&path).into(),
                message: "path is not a regular file".into(),
            });
        }

        let bytes = self.request(&path, "application/vnd.github.raw+json")?;
        let text = String::from_utf8(bytes).map_err(|source| Error::Utf8 {
            path: display_path(&path).into(),
            source,
        })?;
        Ok(slice_text(text, offset.unwrap_or(0), limit))
    }
}

fn parse_repository(source: &str) -> Result<(String, String)> {
    let value = source.trim().trim_end_matches('/');
    let lower = value.to_ascii_lowercase();

    // git@github.com:owner/repository.git and github.com:owner/repository.git
    if let Some(colon) = value.find(':') {
        let host = &value[..colon];
        if !host.contains("//")
            && host
                .rsplit_once('@')
                .map_or(host, |(_, host)| host)
                .eq_ignore_ascii_case("github.com")
        {
            let parts: Vec<_> = value[colon + 1..].split('/').collect();
            if parts.len() == 2 {
                return finish_repository(parts[0], parts[1], source);
            }
        }
    }

    if lower.contains("://") {
        let parsed = Url::parse(value).map_err(|error| Error::InvalidRepository {
            repository: source.into(),
            message: error.to_string(),
        })?;
        if parsed
            .host_str()
            .is_none_or(|host| !host.eq_ignore_ascii_case("github.com"))
        {
            return Err(Error::InvalidRepository {
                repository: source.into(),
                message: "URL host must be github.com".into(),
            });
        }
        let parts: Vec<_> = parsed
            .path_segments()
            .into_iter()
            .flatten()
            .filter(|part| !part.is_empty())
            .collect();
        if parts.len() == 2 {
            return finish_repository(parts[0], parts[1], source);
        }
    } else {
        let parts: Vec<_> = value.split('/').collect();
        if parts.len() == 2 {
            return finish_repository(parts[0], parts[1], source);
        }
    }

    Err(Error::InvalidRepository {
        repository: source.into(),
        message: "expected an owner/repository pair or GitHub repository URL".into(),
    })
}

fn finish_repository(owner: &str, repository: &str, source: &str) -> Result<(String, String)> {
    let owner = owner.to_owned();
    let mut repository = repository.to_owned();
    if repository.to_ascii_lowercase().ends_with(".git") {
        repository.truncate(repository.len() - 4);
    }
    validate_segment_pair(&owner, &repository, source)?;
    Ok((owner, repository))
}

fn validate_segment_pair(owner: &str, repository: &str, source: &str) -> Result<()> {
    if owner.is_empty()
        || repository.is_empty()
        || owner.contains('/')
        || repository.contains('/')
        || matches!(owner, "." | "..")
        || matches!(repository, "." | "..")
    {
        return Err(Error::InvalidRepository {
            repository: source.into(),
            message: "owner and repository must be non-empty path segments".into(),
        });
    }
    Ok(())
}

fn normalize_github_path(relative_path: &str) -> Result<String> {
    if relative_path.starts_with('/') {
        return Err(Error::InvalidPath {
            path: relative_path.into(),
            message: "path must be relative".into(),
        });
    }
    let mut parts = Vec::new();
    for part in relative_path.split('/') {
        match part {
            "" | "." => {}
            ".." => {
                return Err(Error::InvalidPath {
                    path: relative_path.into(),
                    message: "path is outside the accessed repository".into(),
                });
            }
            part => parts.push(part),
        }
    }
    Ok(parts.join("/"))
}

fn display_path(path: &str) -> &str {
    if path.is_empty() { "." } else { path }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[test]
    fn directory_accessor_lists_and_reads_character_ranges() {
        let root = tempfile::tempdir().unwrap();
        fs::create_dir(root.path().join("z-dir")).unwrap();
        let mut file = fs::File::create(root.path().join("a.txt")).unwrap();
        write!(file, "aé日z").unwrap();

        let accessor = DirectoryAccessor::new(root.path()).unwrap();
        assert_eq!(
            accessor.list_dir(".").unwrap(),
            vec![
                DirEntry::file("a.txt", Some(7)),
                DirEntry::directory("z-dir")
            ]
        );
        assert_eq!(
            accessor.read_file("a.txt", Some(1), Some(2)).unwrap(),
            "é日"
        );
    }

    #[test]
    fn directory_accessor_rejects_parent_components() {
        let root = tempfile::tempdir().unwrap();
        let accessor = DirectoryAccessor::new(root.path()).unwrap();
        assert!(accessor.read_file("../secret", None, None).is_err());
    }

    #[test]
    fn parses_supported_github_repository_forms() {
        for source in [
            "octocat/Hello-World",
            "https://github.com/octocat/Hello-World.git",
            "ssh://git@github.com/octocat/Hello-World.git",
            "git@github.com:octocat/Hello-World.git",
        ] {
            let accessor = GitHubAccessor::new(source).unwrap().without_token();
            assert_eq!(accessor.owner(), "octocat");
            assert_eq!(accessor.repository(), "Hello-World");
        }
    }

    #[test]
    fn rejects_non_github_and_traversing_paths() {
        assert!(GitHubAccessor::new("https://example.com/a/b").is_err());
        assert!(normalize_github_path("a/../b").is_err());
        assert_eq!(
            normalize_github_path("./src//lib.rs").unwrap(),
            "src/lib.rs"
        );
    }
}
