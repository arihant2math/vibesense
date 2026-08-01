//! Repository-level AI-authorship estimation.
//!
//! This crate inverts the judge-first design: no LLM chooses what to scan.
//! Stage 1 walks the repository through an [`access::Accessor`] and selects
//! candidate source files with deterministic filters — a code-extension
//! allowlist, vendored-directory and generated-file exclusions, and size
//! limits for pathologically large files. Stage 2 scans the candidates in a
//! seeded pseudorandom order with the ONNX classifier, treating each file as a
//! cluster of chunks, and keeps a running ratio estimate of the fraction of
//! AI-written chunks. The scan stops when the 99% confidence interval is
//! narrower than the configured target, when a chunk budget is exhausted, or
//! when every candidate has been scanned.
//!
//! [`analyze`] awaits the final [`RepoStats`]; [`analyze_streaming`] also
//! exposes a [`tokio::sync::watch`] channel whose value is updated after every
//! sampled file, for live progress in a webservice. [`analyze_blocking`] runs
//! the same scan synchronously on a private single-thread runtime.

mod run;
mod select;
mod stats;

use std::sync::{Arc, Mutex, PoisonError};

use access::Accessor;
use serde::{Deserialize, Serialize};
use tokio::{sync::watch, task::JoinHandle};
use vibesense_classifier::Classifier;

pub use select::SelectionStats;
pub use stats::{Estimate, RepoStats, StopReason};
pub use vibesense_classifier::{Classification, Error as ClassifierError};

/// Critical value for a 99% confidence interval.
pub const Z_99: f64 = 2.575_829_303_548_901;

/// Critical value for a 95% confidence interval.
pub const Z_95: f64 = 1.959_963_984_540_054;

/// Everything the pipeline needs besides the repository and the classifier.
#[derive(Clone, Debug, Default, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct Config {
    pub selection: SelectionConfig,
    pub sampling: SamplingConfig,
}

/// Stage-1 filters deciding which files may be sampled.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct SelectionConfig {
    /// Files larger than this many bytes are skipped outright.
    pub max_file_bytes: u64,
    /// At most this many characters are read from each sampled file, so one
    /// enormous file cannot dominate the scan.
    pub max_read_chars: usize,
    /// The walk stops after selecting this many candidates.
    pub max_files: usize,
    /// Lowercase file extensions treated as source code.
    pub extensions: Vec<String>,
    /// Lowercase directory names pruned from the walk. Hidden directories are
    /// always pruned.
    pub excluded_directories: Vec<String>,
}

impl Default for SelectionConfig {
    fn default() -> Self {
        Self {
            max_file_bytes: 2 * 1024 * 1024,
            max_read_chars: 131_072,
            max_files: 10_000,
            extensions: select::DEFAULT_EXTENSIONS
                .iter()
                .map(|extension| (*extension).to_string())
                .collect(),
            excluded_directories: select::EXCLUDED_DIRECTORIES
                .iter()
                .map(|directory| (*directory).to_string())
                .collect(),
        }
    }
}

/// Stage-2 sampling order, stop conditions, and chunk labeling.
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct SamplingConfig {
    /// Seed for the pseudorandom file order; equal seeds reproduce a scan.
    pub seed: u64,
    /// Critical value for the confidence interval, [`Z_99`] by default.
    pub z: f64,
    /// The scan stops once the interval half-width is at or below this.
    pub target_half_width: f64,
    /// Never stop on precision before scanning this many files; this guards
    /// against the interval collapsing after a few unanimous files.
    pub min_files: usize,
    /// Hard budget on scored chunks.
    pub max_chunks: usize,
    /// A chunk with at least this AI probability counts as AI-written.
    pub chunk_threshold: f32,
}

impl Default for SamplingConfig {
    fn default() -> Self {
        Self {
            seed: 0,
            z: Z_99,
            target_half_width: 0.05,
            min_files: 10,
            max_chunks: 50_000,
            chunk_threshold: 0.5,
        }
    }
}

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("could not list the repository root: {0}")]
    RootAccess(#[from] access::Error),

    #[error("the analysis task failed: {0}")]
    Worker(#[from] tokio::task::JoinError),

    #[error("could not start the analysis runtime: {0}")]
    Runtime(#[from] std::io::Error),
}

/// Anything that can score one source file.
///
/// Implemented for [`Classifier`] and for `Arc<Mutex<Classifier>>`, so a
/// webservice can share one loaded model across scans.
pub trait FileClassifier: Send {
    fn classify_file(&mut self, code: &str) -> Result<Classification, ClassifierError>;
}

impl FileClassifier for Classifier {
    fn classify_file(&mut self, code: &str) -> Result<Classification, ClassifierError> {
        self.classify(code)
    }
}

impl FileClassifier for Arc<Mutex<Classifier>> {
    fn classify_file(&mut self, code: &str) -> Result<Classification, ClassifierError> {
        self.lock()
            .unwrap_or_else(PoisonError::into_inner)
            .classify(code)
    }
}

/// A scan running on Tokio's blocking pool.
///
/// Dropping the `Analysis` and every receiver obtained from [`progress`]
/// cancels the scan at the next file boundary.
///
/// [`progress`]: Analysis::progress
pub struct Analysis {
    progress: watch::Receiver<RepoStats>,
    handle: JoinHandle<Result<RepoStats, Error>>,
}

impl Analysis {
    /// Subscribe to live statistics; the value is replaced after selection and
    /// after every sampled file, and holds the final statistics once the scan
    /// ends.
    pub fn progress(&self) -> watch::Receiver<RepoStats> {
        self.progress.clone()
    }

    /// Wait for the scan to finish and return the final statistics.
    pub async fn wait(self) -> Result<RepoStats, Error> {
        let Self { progress, handle } = self;
        let result = handle.await?;
        // Hold the receiver until the scan has ended so waiting alone never
        // cancels it.
        drop(progress);
        result
    }
}

/// Scan a repository and return the final statistics.
pub async fn analyze<C>(
    accessor: Arc<dyn Accessor>,
    classifier: C,
    config: Config,
) -> Result<RepoStats, Error>
where
    C: FileClassifier + 'static,
{
    analyze_streaming(accessor, classifier, config).wait().await
}

/// Scan a repository with live progress updates.
pub fn analyze_streaming<C>(accessor: Arc<dyn Accessor>, classifier: C, config: Config) -> Analysis
where
    C: FileClassifier + 'static,
{
    let (sender, receiver) = watch::channel(RepoStats::default());
    // The scan stays on the blocking pool so classification never stalls an
    // executor thread; accessor I/O is driven back on the runtime.
    let runtime = tokio::runtime::Handle::current();
    let handle = tokio::task::spawn_blocking(move || {
        let mut classifier = classifier;
        let result = runtime.block_on(run::run(
            accessor.as_ref(),
            &mut classifier,
            &config,
            |current| {
                sender.send_replace(current.clone());
                !sender.is_closed()
            },
        ));
        if let Ok(final_stats) = &result {
            sender.send_replace(final_stats.clone());
        }
        result
    });
    Analysis {
        progress: receiver,
        handle,
    }
}

/// Scan a repository synchronously on a private single-thread runtime.
///
/// Must not be called from within a Tokio runtime; use [`analyze`] there.
pub fn analyze_blocking<A, C>(
    accessor: &A,
    classifier: &mut C,
    config: &Config,
) -> Result<RepoStats, Error>
where
    A: Accessor + ?Sized,
    C: FileClassifier + ?Sized,
{
    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()?;
    runtime.block_on(run::run(accessor, classifier, config, |_| true))
}

#[cfg(test)]
pub(crate) mod testing {
    use std::collections::{BTreeMap, BTreeSet};

    use access::{Accessor, DirEntry, Error as AccessError, Result as AccessResult, async_trait};
    use vibesense_classifier::{Aggregation, Classification, Prediction};

    use crate::{ClassifierError, FileClassifier};

    /// An in-memory repository built from `path -> content` pairs.
    pub struct MapAccessor {
        files: BTreeMap<String, String>,
    }

    impl MapAccessor {
        pub fn new<P, C>(files: impl IntoIterator<Item = (P, C)>) -> Self
        where
            P: Into<String>,
            C: Into<String>,
        {
            Self {
                files: files
                    .into_iter()
                    .map(|(path, content)| (path.into(), content.into()))
                    .collect(),
            }
        }
    }

    #[async_trait]
    impl Accessor for MapAccessor {
        async fn list_dir(&self, relative_path: &str) -> AccessResult<Vec<DirEntry>> {
            let prefix = if relative_path == "." {
                String::new()
            } else {
                format!("{relative_path}/")
            };
            let mut directories = BTreeSet::new();
            let mut entries = Vec::new();
            for (path, content) in &self.files {
                let Some(rest) = path.strip_prefix(&prefix) else {
                    continue;
                };
                match rest.split_once('/') {
                    Some((child, _)) => {
                        directories.insert(child.to_string());
                    }
                    None => entries.push(DirEntry::file(rest, Some(content.len() as u64))),
                }
            }
            if directories.is_empty() && entries.is_empty() {
                return Err(AccessError::NotFound(relative_path.into()));
            }
            entries.extend(directories.into_iter().map(DirEntry::directory));
            entries.sort_by(|left, right| left.name.cmp(&right.name));
            Ok(entries)
        }

        async fn read_file(
            &self,
            relative_path: &str,
            offset: Option<usize>,
            limit: Option<usize>,
        ) -> AccessResult<String> {
            let content = self
                .files
                .get(relative_path)
                .ok_or_else(|| AccessError::NotFound(relative_path.into()))?;
            Ok(content
                .chars()
                .skip(offset.unwrap_or(0))
                .take(limit.unwrap_or(usize::MAX))
                .collect())
        }
    }

    /// Scores one chunk per line: a line that is exactly `ai` scores 0.9,
    /// anything else 0.1.
    pub struct FakeClassifier;

    impl FileClassifier for FakeClassifier {
        fn classify_file(&mut self, code: &str) -> Result<Classification, ClassifierError> {
            let chunk_ai_probabilities: Vec<f32> = code
                .lines()
                .filter(|line| !line.trim().is_empty())
                .map(|line| if line.trim() == "ai" { 0.9 } else { 0.1 })
                .collect();
            if chunk_ai_probabilities.is_empty() {
                return Err(ClassifierError::EmptyInput);
            }
            let ai_probability =
                chunk_ai_probabilities.iter().sum::<f32>() / chunk_ai_probabilities.len() as f32;
            Ok(Classification {
                prediction: if ai_probability >= 0.5 {
                    Prediction::Ai
                } else {
                    Prediction::Human
                },
                ai_probability,
                human_probability: 1.0 - ai_probability,
                chunks: chunk_ai_probabilities.len(),
                chunk_ai_probabilities,
                aggregation: Aggregation::Mean,
            })
        }
    }
}

#[cfg(test)]
mod tests {
    use std::sync::Arc;

    use super::*;
    use crate::testing::{FakeClassifier, MapAccessor};

    fn accessor() -> Arc<dyn Accessor> {
        Arc::new(MapAccessor::new([
            ("src/a.rs", "ai\nai\n"),
            ("src/b.rs", "human\nhuman\nhuman\n"),
            ("src/c.rs", "human\nai\n"),
        ]))
    }

    #[tokio::test]
    async fn analyze_returns_final_stats() {
        let stats = analyze(accessor(), FakeClassifier, Config::default())
            .await
            .unwrap();
        assert_eq!(stats.stop_reason, Some(StopReason::AllFilesScanned));
        assert_eq!(stats.files_scanned, 3);
        assert_eq!(stats.chunks_ai, 3);
        assert_eq!(stats.chunks_human, 4);
    }

    #[tokio::test]
    async fn streaming_progress_converges_to_the_final_stats() {
        let analysis = analyze_streaming(accessor(), FakeClassifier, Config::default());
        let mut progress = analysis.progress();
        let stats = analysis.wait().await.unwrap();
        assert!(stats.stop_reason.is_some());
        assert_eq!(*progress.borrow_and_update(), stats);
    }

    #[tokio::test]
    async fn analyze_fails_when_the_root_cannot_be_listed() {
        let empty: Arc<dyn Accessor> = Arc::new(MapAccessor::new(Vec::<(String, String)>::new()));
        let error = analyze(empty, FakeClassifier, Config::default())
            .await
            .unwrap_err();
        assert!(matches!(error, Error::RootAccess(_)));
    }
}
