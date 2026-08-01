//! The synchronous scan loop shared by the blocking and streaming APIs.

use access::Accessor;
use vibesense_classifier::Prediction;

use crate::{
    Config, Error, FileClassifier,
    select::{Candidate, looks_generated, select_files},
    stats::{Estimator, RepoStats, SplitMix64, StopReason, shuffle},
};

/// Select candidates, scan them in seeded pseudorandom order, and stop as soon
/// as a stop condition holds.
///
/// `on_progress` is called with the updated statistics after selection and
/// after every sampled file; returning `false` cancels the scan at the next
/// file boundary.
pub(crate) fn run<A, C>(
    accessor: &A,
    classifier: &mut C,
    config: &Config,
    mut on_progress: impl FnMut(&RepoStats) -> bool,
) -> Result<RepoStats, Error>
where
    A: Accessor + ?Sized,
    C: FileClassifier + ?Sized,
{
    let (mut candidates, selection) = select_files(accessor, &config.selection)?;
    shuffle(&mut candidates, &mut SplitMix64::new(config.sampling.seed));

    let mut stats = RepoStats {
        selection,
        ..RepoStats::default()
    };
    let mut estimator = Estimator::new(candidates.len());
    let mut probability_sum = 0.0_f64;

    if !on_progress(&stats) {
        stats.stop_reason = Some(StopReason::Cancelled);
        return Ok(stats);
    }

    for candidate in &candidates {
        scan_file(
            accessor,
            classifier,
            candidate,
            config,
            &mut stats,
            &mut estimator,
            &mut probability_sum,
        );
        stats.estimate = estimator.estimate(config.sampling.z);

        if let Some(estimate) = &stats.estimate
            && stats.files_scanned >= config.sampling.min_files
            && estimate.half_width <= config.sampling.target_half_width
        {
            stats.stop_reason = Some(StopReason::PrecisionReached);
        }
        if stats.stop_reason.is_none() && stats.chunks_scanned >= config.sampling.max_chunks {
            stats.stop_reason = Some(StopReason::ChunkBudgetExhausted);
        }
        let keep_going = on_progress(&stats);
        if stats.stop_reason.is_none() && !keep_going {
            stats.stop_reason = Some(StopReason::Cancelled);
        }
        if stats.stop_reason.is_some() {
            return Ok(stats);
        }
    }

    stats.stop_reason = Some(StopReason::AllFilesScanned);
    Ok(stats)
}

fn scan_file<A, C>(
    accessor: &A,
    classifier: &mut C,
    candidate: &Candidate,
    config: &Config,
    stats: &mut RepoStats,
    estimator: &mut Estimator,
    probability_sum: &mut f64,
) where
    A: Accessor + ?Sized,
    C: FileClassifier + ?Sized,
{
    let content =
        match accessor.read_file(&candidate.path, None, Some(config.selection.max_read_chars)) {
            Ok(content) => content,
            Err(_) => {
                stats.files_errored += 1;
                estimator.exclude();
                return;
            }
        };
    if looks_generated(&content) {
        stats.files_skipped_generated += 1;
        estimator.exclude();
        return;
    }
    let classification = match classifier.classify_file(&content) {
        Ok(classification) => classification,
        Err(_) => {
            stats.files_errored += 1;
            estimator.exclude();
            return;
        }
    };

    let chunk_count = classification.chunk_ai_probabilities.len();
    let ai_chunks = classification
        .chunk_ai_probabilities
        .iter()
        .filter(|probability| **probability >= config.sampling.chunk_threshold)
        .count();

    stats.files_scanned += 1;
    match classification.prediction {
        Prediction::Ai => stats.files_ai += 1,
        Prediction::Human => stats.files_human += 1,
    }
    stats.chunks_scanned += chunk_count;
    stats.chunks_ai += ai_chunks;
    stats.chunks_human += chunk_count - ai_chunks;
    *probability_sum += classification
        .chunk_ai_probabilities
        .iter()
        .map(|probability| f64::from(*probability))
        .sum::<f64>();
    if stats.chunks_scanned > 0 {
        stats.mean_chunk_ai_probability = Some(*probability_sum / stats.chunks_scanned as f64);
    }
    estimator.record(chunk_count, ai_chunks);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::{FakeClassifier, MapAccessor};
    use crate::{Config, SamplingConfig};

    fn scan(accessor: &MapAccessor, config: &Config) -> RepoStats {
        run(accessor, &mut FakeClassifier, config, |_| true).unwrap()
    }

    #[test]
    fn a_full_scan_counts_files_and_chunks() {
        let accessor = MapAccessor::new([
            ("a.rs", "ai\nai\nai\n"),
            ("b.rs", "human\nhuman\n"),
            ("c.rs", "human\nai\nhuman\nhuman\n"),
        ]);
        let config = Config {
            sampling: SamplingConfig {
                // Forbid an early precision stop so the scan is a census.
                min_files: usize::MAX,
                ..SamplingConfig::default()
            },
            ..Config::default()
        };

        let stats = scan(&accessor, &config);
        assert_eq!(stats.stop_reason, Some(StopReason::AllFilesScanned));
        assert_eq!(stats.files_scanned, 3);
        assert_eq!(stats.files_ai, 1);
        assert_eq!(stats.files_human, 2);
        assert_eq!(stats.chunks_scanned, 9);
        assert_eq!(stats.chunks_ai, 4);
        assert_eq!(stats.chunks_human, 5);

        let estimate = stats.estimate.unwrap();
        assert!((estimate.ai_chunk_fraction - 4.0 / 9.0).abs() < 1e-12);
        assert_eq!(estimate.half_width, 0.0);

        let mean = stats.mean_chunk_ai_probability.unwrap();
        assert!((mean - (4.0 * 0.9 + 5.0 * 0.1) / 9.0).abs() < 1e-6);
    }

    #[test]
    fn homogeneous_repositories_stop_early_on_precision() {
        let files: Vec<(String, String)> = (0..30)
            .map(|index| {
                (
                    format!("file{index}.rs"),
                    String::from("human\nhuman\nhuman\n"),
                )
            })
            .collect();
        let accessor = MapAccessor::new(files);

        let stats = scan(&accessor, &Config::default());
        assert_eq!(stats.stop_reason, Some(StopReason::PrecisionReached));
        assert_eq!(stats.files_scanned, SamplingConfig::default().min_files);
        assert_eq!(stats.estimate.unwrap().ai_chunk_fraction, 0.0);
    }

    #[test]
    fn the_chunk_budget_stops_the_scan() {
        let accessor = MapAccessor::new([
            ("a.rs", "ai\nai\n"),
            ("b.rs", "human\n"),
            ("c.rs", "human\n"),
        ]);
        let config = Config {
            sampling: SamplingConfig {
                max_chunks: 1,
                min_files: usize::MAX,
                ..SamplingConfig::default()
            },
            ..Config::default()
        };

        let stats = scan(&accessor, &config);
        assert_eq!(stats.stop_reason, Some(StopReason::ChunkBudgetExhausted));
        assert_eq!(stats.files_scanned, 1);
    }

    #[test]
    fn generated_content_is_skipped_at_scan_time() {
        let accessor = MapAccessor::new([
            (
                "gen.rs",
                "// Code generated by bindgen. DO NOT EDIT.\nhuman\n",
            ),
            ("real.rs", "human\nhuman\n"),
        ]);
        let config = Config {
            sampling: SamplingConfig {
                min_files: usize::MAX,
                ..SamplingConfig::default()
            },
            ..Config::default()
        };

        let stats = scan(&accessor, &config);
        assert_eq!(stats.files_skipped_generated, 1);
        assert_eq!(stats.files_scanned, 1);
        // The census still collapses because the population shrank with the skip.
        assert_eq!(stats.estimate.unwrap().half_width, 0.0);
    }

    #[test]
    fn cancellation_from_the_progress_callback_is_reported() {
        let accessor = MapAccessor::new([("a.rs", "human\n"), ("b.rs", "human\n")]);
        let mut calls = 0;
        let stats = run(&accessor, &mut FakeClassifier, &Config::default(), |_| {
            calls += 1;
            calls <= 1
        })
        .unwrap();
        assert_eq!(stats.stop_reason, Some(StopReason::Cancelled));
        assert!(stats.files_scanned <= 1);
    }

    #[test]
    fn an_empty_repository_finishes_with_no_estimate() {
        let accessor = MapAccessor::new([("README.md", "docs\n")]);
        let stats = scan(&accessor, &Config::default());
        assert_eq!(stats.stop_reason, Some(StopReason::AllFilesScanned));
        assert_eq!(stats.files_scanned, 0);
        assert!(stats.estimate.is_none());
    }
}
