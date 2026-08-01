use std::path::{Component, Path, PathBuf};

use tokio::fs;

use crate::{Accessor, DirEntry, Error, Result, async_trait, text::slice_text};

/// Access files below a directory on the local filesystem.
#[derive(Clone, Debug)]
pub struct DirectoryAccessor {
    directory: PathBuf,
}

impl DirectoryAccessor {
    pub fn new(directory: impl AsRef<Path>) -> Result<Self> {
        let supplied = directory.as_ref();
        let directory = std::fs::canonicalize(supplied).map_err(|source| Error::Io {
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

    async fn resolve(&self, relative_path: &str) -> Result<PathBuf> {
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
        let resolved = fs::canonicalize(&unresolved)
            .await
            .map_err(|source| Error::Io {
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

#[async_trait]
impl Accessor for DirectoryAccessor {
    async fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>> {
        let directory = self.resolve(relative_path).await?;
        if !directory.is_dir() {
            return Err(io_error(
                directory,
                std::io::ErrorKind::NotADirectory,
                "path is not a directory",
            ));
        }

        let mut paths = fs::read_dir(&directory).await.map_err(|source| Error::Io {
            path: directory.clone(),
            source,
        })?;
        let mut result = Vec::new();
        loop {
            let entry = paths.next_entry().await.map_err(|source| Error::Io {
                path: directory.clone(),
                source,
            })?;
            let Some(entry) = entry else {
                break;
            };
            let path = entry.path();

            // Broken links and links whose targets escape the root are not exposed.
            let Ok(resolved) = fs::canonicalize(&path).await else {
                continue;
            };
            if !resolved.starts_with(&self.directory) {
                continue;
            }
            let Ok(metadata) = fs::metadata(&resolved).await else {
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

    async fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String> {
        let path = self.resolve(relative_path).await?;
        if !path.is_file() {
            let kind = if path.is_dir() {
                std::io::ErrorKind::IsADirectory
            } else {
                std::io::ErrorKind::InvalidInput
            };
            return Err(io_error(path, kind, "path is not a regular file"));
        }

        let bytes = fs::read(&path).await.map_err(|source| Error::Io {
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

#[cfg(test)]
mod tests {
    use std::{fs, io::Write};

    use super::*;

    #[tokio::test]
    async fn lists_and_reads_character_ranges() {
        let root = tempfile::tempdir().unwrap();
        fs::create_dir(root.path().join("z-dir")).unwrap();
        let mut file = fs::File::create(root.path().join("a.txt")).unwrap();
        write!(file, "aé日z").unwrap();

        let accessor = DirectoryAccessor::new(root.path()).unwrap();
        assert_eq!(
            accessor.list_dir(".").await.unwrap(),
            vec![
                DirEntry::file("a.txt", Some(7)),
                DirEntry::directory("z-dir")
            ]
        );
        assert_eq!(
            accessor.read_file("a.txt", Some(1), Some(2)).await.unwrap(),
            "é日"
        );
    }

    #[tokio::test]
    async fn rejects_parent_components() {
        let root = tempfile::tempdir().unwrap();
        let accessor = DirectoryAccessor::new(root.path()).unwrap();
        assert!(accessor.read_file("../secret", None, None).await.is_err());
    }
}
