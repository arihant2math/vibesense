use std::path::PathBuf;

use reqwest::StatusCode;

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
    Http(#[from] reqwest::Error),

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
