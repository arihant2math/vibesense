//! Stage 2 statistics: sampling order and the sequential estimator.
//!
//! Files are sampled without replacement in a seeded pseudorandom order. Each
//! scanned file contributes its classifier chunks as one cluster, so the
//! fraction of AI-written chunks is estimated with the ratio estimator for
//! single-stage cluster sampling, including a finite population correction:
//! once every candidate has been scanned the interval collapses to a point.
//!
//! The interval uses a normal approximation, which is optimistic near
//! fractions of exactly 0 or 1; `min_files` exists so a handful of
//! early, unanimous files cannot end the scan on their own.

use serde::Serialize;

use crate::SelectionStats;

/// A point estimate of the AI-written chunk fraction with its confidence interval.
#[derive(Clone, Copy, Debug, PartialEq, Serialize)]
pub struct Estimate {
    /// Estimated fraction of all candidate chunks that are AI-written.
    pub ai_chunk_fraction: f64,
    /// Lower bound of the confidence interval, clamped to `[0, 1]`.
    pub low: f64,
    /// Upper bound of the confidence interval, clamped to `[0, 1]`.
    pub high: f64,
    /// Half-width of the unclamped interval; the scan stops when this drops
    /// below the configured target.
    pub half_width: f64,
    /// The critical value the interval was built with.
    pub z: f64,
}

/// Why the scan ended.
#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum StopReason {
    /// The confidence interval reached the configured half-width.
    PrecisionReached,
    /// Every candidate file was scanned; the estimate is a census.
    AllFilesScanned,
    /// The configured chunk budget was spent first.
    ChunkBudgetExhausted,
    /// Every progress subscriber was dropped, so the scan stopped early.
    Cancelled,
}

/// Live statistics for one repository scan.
///
/// This is both the streaming progress value and the final report; a finished
/// scan is marked by `stop_reason` being set.
#[derive(Clone, Debug, Default, PartialEq, Serialize)]
pub struct RepoStats {
    /// What stage 1 selected and why the rest was dropped.
    pub selection: SelectionStats,
    /// Files scored by the classifier so far.
    pub files_scanned: usize,
    /// Scanned files whose aggregate prediction was AI.
    pub files_ai: usize,
    /// Scanned files whose aggregate prediction was human.
    pub files_human: usize,
    /// Sampled files skipped at read time because their content looks generated.
    pub files_skipped_generated: usize,
    /// Sampled files that could not be read or classified.
    pub files_errored: usize,
    /// Classifier chunks scored so far.
    pub chunks_scanned: usize,
    /// Chunks at or above the AI probability threshold.
    pub chunks_ai: usize,
    /// Chunks below the AI probability threshold.
    pub chunks_human: usize,
    /// Mean AI probability over all scored chunks.
    pub mean_chunk_ai_probability: Option<f64>,
    /// Current estimate of the repository's AI-written chunk fraction.
    pub estimate: Option<Estimate>,
    /// Set once the scan has finished.
    pub stop_reason: Option<StopReason>,
}

/// Sequential ratio estimator over sampled file clusters.
pub(crate) struct Estimator {
    clusters: Vec<Cluster>,
    /// Candidate files remaining in the population; shrinks when a sampled
    /// file turns out not to belong (generated content, read errors).
    population: usize,
}

struct Cluster {
    chunks: f64,
    ai_chunks: f64,
}

impl Estimator {
    pub fn new(population: usize) -> Self {
        Self {
            clusters: Vec::new(),
            population,
        }
    }

    /// Remove a sampled file from the population without recording chunks.
    pub fn exclude(&mut self) {
        self.population = self.population.saturating_sub(1);
    }

    /// Record one scanned file.
    pub fn record(&mut self, chunks: usize, ai_chunks: usize) {
        self.clusters.push(Cluster {
            chunks: chunks as f64,
            ai_chunks: ai_chunks as f64,
        });
    }

    /// The current estimate, or `None` while too little has been scanned to
    /// bound the error (fewer than two files, unless the sample is a census).
    pub fn estimate(&self, z: f64) -> Option<Estimate> {
        let sampled = self.clusters.len();
        let total_chunks: f64 = self.clusters.iter().map(|cluster| cluster.chunks).sum();
        if total_chunks <= 0.0 {
            return None;
        }
        let total_ai: f64 = self.clusters.iter().map(|cluster| cluster.ai_chunks).sum();
        let fraction = total_ai / total_chunks;

        let half_width = if sampled >= self.population {
            0.0
        } else if sampled < 2 {
            return None;
        } else {
            let count = sampled as f64;
            let mean_size = total_chunks / count;
            let residual_sum: f64 = self
                .clusters
                .iter()
                .map(|cluster| {
                    let residual = cluster.ai_chunks - fraction * cluster.chunks;
                    residual * residual
                })
                .sum();
            let variance = (1.0 - count / self.population as f64) * residual_sum
                / ((count - 1.0) * count * mean_size * mean_size);
            z * variance.max(0.0).sqrt()
        };

        Some(Estimate {
            ai_chunk_fraction: fraction,
            low: (fraction - half_width).max(0.0),
            high: (fraction + half_width).min(1.0),
            half_width,
            z,
        })
    }
}

/// SplitMix64: a small, seedable generator so sampling is reproducible
/// without an extra dependency.
pub(crate) struct SplitMix64(u64);

impl SplitMix64 {
    pub fn new(seed: u64) -> Self {
        Self(seed)
    }

    fn next_u64(&mut self) -> u64 {
        self.0 = self.0.wrapping_add(0x9E37_79B9_7F4A_7C15);
        let mut value = self.0;
        value = (value ^ (value >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
        value = (value ^ (value >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
        value ^ (value >> 31)
    }

    fn next_below(&mut self, bound: u64) -> u64 {
        ((u128::from(self.next_u64()) * u128::from(bound)) >> 64) as u64
    }
}

/// Fisher-Yates shuffle driven by [`SplitMix64`].
pub(crate) fn shuffle<T>(items: &mut [T], rng: &mut SplitMix64) {
    for index in (1..items.len()).rev() {
        let other = rng.next_below(index as u64 + 1) as usize;
        items.swap(index, other);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn shuffle_is_deterministic_per_seed_and_permutes() {
        let mut first: Vec<u32> = (0..100).collect();
        let mut second: Vec<u32> = (0..100).collect();
        shuffle(&mut first, &mut SplitMix64::new(7));
        shuffle(&mut second, &mut SplitMix64::new(7));
        assert_eq!(first, second);

        let mut other_seed: Vec<u32> = (0..100).collect();
        shuffle(&mut other_seed, &mut SplitMix64::new(8));
        assert_ne!(first, other_seed);

        let mut sorted = first.clone();
        sorted.sort_unstable();
        assert_eq!(sorted, (0..100).collect::<Vec<u32>>());
    }

    #[test]
    fn a_census_has_zero_half_width() {
        let mut estimator = Estimator::new(2);
        estimator.record(10, 5);
        estimator.record(30, 30);
        let estimate = estimator.estimate(crate::Z_99).unwrap();
        assert_eq!(estimate.half_width, 0.0);
        assert!((estimate.ai_chunk_fraction - 0.875).abs() < 1e-12);
        assert_eq!(estimate.low, estimate.high);
    }

    #[test]
    fn a_single_cluster_from_a_larger_population_has_no_estimate() {
        let mut estimator = Estimator::new(5);
        estimator.record(10, 5);
        assert!(estimator.estimate(crate::Z_99).is_none());
    }

    #[test]
    fn variance_matches_the_ratio_estimator_by_hand() {
        // Two clusters of 10 chunks from a population of 1000 files:
        // p = 0.5, residuals ±5, s² = 50, m̄ = 10,
        // var = (1 - 2/1000) · 50 / (1 · 2 · 100) = 0.2495.
        let mut estimator = Estimator::new(1000);
        estimator.record(10, 10);
        estimator.record(10, 0);
        let estimate = estimator.estimate(crate::Z_99).unwrap();
        assert!((estimate.ai_chunk_fraction - 0.5).abs() < 1e-12);
        let expected = crate::Z_99 * 0.2495_f64.sqrt();
        assert!((estimate.half_width - expected).abs() < 1e-9);
        assert_eq!(estimate.low, 0.0);
        assert_eq!(estimate.high, 1.0);
    }

    #[test]
    fn identical_clusters_collapse_the_interval() {
        let mut estimator = Estimator::new(100);
        for _ in 0..5 {
            estimator.record(8, 2);
        }
        let estimate = estimator.estimate(crate::Z_99).unwrap();
        assert!((estimate.ai_chunk_fraction - 0.25).abs() < 1e-12);
        assert_eq!(estimate.half_width, 0.0);
    }

    #[test]
    fn empty_or_chunkless_samples_have_no_estimate() {
        let estimator = Estimator::new(10);
        assert!(estimator.estimate(crate::Z_99).is_none());

        let mut chunkless = Estimator::new(10);
        chunkless.record(0, 0);
        chunkless.record(0, 0);
        assert!(chunkless.estimate(crate::Z_99).is_none());
    }
}
