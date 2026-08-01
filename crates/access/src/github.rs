use std::{env, io, time::Duration};

use reqwest::{Client, StatusCode};
use serde_json::Value;
use url::Url;

use crate::{Accessor, DirEntry, Error, Result, async_trait, text::slice_text};

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
    client: Client,
}

fn github_client(timeout: Duration) -> Result<Client> {
    Client::builder()
        .timeout(timeout)
        .build()
        .map_err(Error::Http)
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

        let timeout = Duration::from_secs(30);
        Ok(Self {
            owner,
            repository,
            git_ref: None,
            token: env::var("GITHUB_TOKEN")
                .ok()
                .or_else(|| env::var("GH_TOKEN").ok()),
            api_url: Url::parse("https://api.github.com").expect("constant GitHub URL is valid"),
            client: github_client(timeout)?,
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
        self.client = github_client(timeout)?;
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

    async fn request(&self, path: &str, accept: &'static str) -> Result<Vec<u8>> {
        let url = self.contents_url(path)?;
        let mut request = self
            .client
            .get(url)
            .header("accept", accept)
            .header("user-agent", "vibesense")
            .header("x-github-api-version", "2022-11-28");
        if let Some(token) = &self.token {
            request = request.header("authorization", format!("Bearer {token}"));
        }
        if let Some(git_ref) = &self.git_ref {
            request = request.query(&[("ref", git_ref)]);
        }

        let response = request.send().await?;
        let status = response.status();
        let bytes = response.bytes().await?.to_vec();
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

    async fn metadata(&self, path: &str) -> Result<Value> {
        let bytes = self.request(path, "application/vnd.github+json").await?;
        let value: Value = serde_json::from_slice(&bytes)?;
        if !value.is_array() && !value.is_object() {
            return Err(Error::Metadata(serde_json::Error::io(io::Error::new(
                io::ErrorKind::InvalidData,
                "unexpected repository metadata",
            ))));
        }
        Ok(value)
    }
}

#[async_trait]
impl Accessor for GitHubAccessor {
    async fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>> {
        let path = normalize_path(relative_path)?;
        let metadata = self.metadata(&path).await?;
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

    async fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String> {
        let path = normalize_path(relative_path)?;
        let metadata = self.metadata(&path).await?;
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

        let bytes = self
            .request(&path, "application/vnd.github.raw+json")
            .await?;
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

fn normalize_path(relative_path: &str) -> Result<String> {
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

    #[test]
    fn parses_supported_repository_forms() {
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
        assert!(normalize_path("a/../b").is_err());
        assert_eq!(normalize_path("./src//lib.rs").unwrap(), "src/lib.rs");
    }
}
