//! Read-only access to a repository on disk or through the GitHub Contents API.

mod directory;
mod error;
mod github;
mod text;

pub use directory::DirectoryAccessor;
pub use error::{Error, Result};
pub use github::GitHubAccessor;

/// Re-exported so implementors of [`Accessor`] use the same macro version.
pub use async_trait::async_trait;
use serde::{Deserialize, Serialize};

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

/// A read-only view of a repository.
///
/// Paths are UTF-8, relative to the repository root, and use `/` as their
/// separator. Implementations must not allow a path to escape that root.
#[async_trait]
pub trait Accessor: Send + Sync {
    /// List one directory, sorted by entry name.
    async fn list_dir(&self, relative_path: &str) -> Result<Vec<DirEntry>>;

    /// Read a UTF-8 file, optionally selecting a range measured in Unicode
    /// scalar values (the same semantics as Python text-file offsets).
    async fn read_file(
        &self,
        relative_path: &str,
        offset: Option<usize>,
        limit: Option<usize>,
    ) -> Result<String>;
}
