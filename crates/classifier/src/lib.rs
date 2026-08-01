//! ONNX inference for the Vibesense code-authorship classifier.
//!
//! Load an artifact produced by `export_onnx.py`, then call [`Classifier::classify`]:
//!
//! ```no_run
//! use vibesense_classifier::Classifier;
//!
//! # fn main() -> Result<(), vibesense_classifier::Error> {
//! let mut classifier = Classifier::from_dir("detector-onnx")?;
//! let result = classifier.classify("fn main() { println!(\"hello\"); }")?;
//! println!("{}: {:.2}% AI", result.prediction, result.ai_probability * 100.0);
//! # Ok(())
//! }
//! ```

use std::{
    fs,
    path::{Path, PathBuf},
};

use ort::{
    session::{Session, builder::SessionBuilder},
    value::Tensor,
};
use serde::{Deserialize, Serialize};
use tokenizers::Tokenizer;
use unicode_normalization::UnicodeNormalization;

const CONFIG_FILE: &str = "classifier_config.json";
const LOGITS_PER_ITEM: usize = 2;

/// How scores from overlapping token windows are combined.
#[derive(Clone, Copy, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Aggregation {
    #[default]
    Mean,
    Max,
}

/// Portable metadata stored next to the ONNX model and tokenizer.
#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub struct ArtifactConfig {
    #[serde(default = "default_model_file")]
    pub model_file: PathBuf,
    #[serde(default = "default_tokenizer_file")]
    pub tokenizer_file: PathBuf,
    pub pad_token_id: u32,
    #[serde(default = "default_max_length")]
    pub max_length: usize,
    #[serde(default = "default_stride")]
    pub stride: usize,
    #[serde(default = "default_batch_size")]
    pub batch_size: usize,
    #[serde(default = "default_threshold")]
    pub threshold: f32,
    #[serde(default)]
    pub aggregation: Aggregation,
}

fn default_model_file() -> PathBuf {
    "model.onnx".into()
}
fn default_tokenizer_file() -> PathBuf {
    "tokenizer.json".into()
}
fn default_max_length() -> usize {
    1024
}
fn default_stride() -> usize {
    128
}
fn default_batch_size() -> usize {
    1
}
fn default_threshold() -> f32 {
    0.5
}

/// A file-level code-authorship prediction.
#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Classification {
    pub prediction: Prediction,
    pub ai_probability: f32,
    pub human_probability: f32,
    pub chunks: usize,
    pub chunk_ai_probabilities: Vec<f32>,
    pub aggregation: Aggregation,
}

/// The predicted author type.
#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Prediction {
    Human,
    Ai,
}

impl std::fmt::Display for Prediction {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Human => formatter.write_str("human"),
            Self::Ai => formatter.write_str("ai"),
        }
    }
}

/// Errors returned while loading or running the classifier.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("failed to read {path}: {source}")]
    Read {
        path: PathBuf,
        #[source]
        source: std::io::Error,
    },
    #[error("invalid classifier config at {path}: {source}")]
    Config {
        path: PathBuf,
        #[source]
        source: serde_json::Error,
    },
    #[error("invalid classifier settings: {0}")]
    InvalidSettings(String),
    #[error("failed to load tokenizer at {path}: {message}")]
    Tokenizer { path: PathBuf, message: String },
    #[error("failed to encode input: {0}")]
    Encode(String),
    #[error("failed to load or run ONNX model: {0}")]
    Onnx(#[from] ort::Error),
    #[error("code must contain non-empty text without NUL characters")]
    EmptyInput,
    #[error("model returned invalid logits: {0}")]
    InvalidOutput(String),
}

/// Builder for loading a classifier from explicit model and tokenizer paths.
pub struct ClassifierBuilder {
    model_path: PathBuf,
    tokenizer_path: PathBuf,
    pad_token_id: Option<u32>,
    max_length: usize,
    stride: usize,
    batch_size: usize,
    threshold: f32,
    aggregation: Aggregation,
    session_builder: Option<SessionBuilder>,
}

impl ClassifierBuilder {
    pub fn new(model_path: impl Into<PathBuf>, tokenizer_path: impl Into<PathBuf>) -> Self {
        Self {
            model_path: model_path.into(),
            tokenizer_path: tokenizer_path.into(),
            pad_token_id: None,
            max_length: default_max_length(),
            stride: default_stride(),
            batch_size: default_batch_size(),
            threshold: default_threshold(),
            aggregation: Aggregation::default(),
            session_builder: None,
        }
    }

    pub fn pad_token_id(mut self, id: u32) -> Self {
        self.pad_token_id = Some(id);
        self
    }

    pub fn max_length(mut self, value: usize) -> Self {
        self.max_length = value;
        self
    }

    pub fn stride(mut self, value: usize) -> Self {
        self.stride = value;
        self
    }

    pub fn batch_size(mut self, value: usize) -> Self {
        self.batch_size = value;
        self
    }

    pub fn threshold(mut self, value: f32) -> Self {
        self.threshold = value;
        self
    }

    pub fn aggregation(mut self, value: Aggregation) -> Self {
        self.aggregation = value;
        self
    }

    /// Use a customized ONNX Runtime session builder, for example to select a
    /// CUDA or CoreML execution provider.
    pub fn session_builder(mut self, value: SessionBuilder) -> Self {
        self.session_builder = Some(value);
        self
    }

    pub fn build(self) -> Result<Classifier, Error> {
        validate_settings(
            self.max_length,
            self.stride,
            self.batch_size,
            self.threshold,
        )?;

        let tokenizer =
            Tokenizer::from_file(&self.tokenizer_path).map_err(|error| Error::Tokenizer {
                path: self.tokenizer_path.clone(),
                message: error.to_string(),
            })?;
        let pad_token_id = self
            .pad_token_id
            .or_else(|| tokenizer.get_padding().map(|padding| padding.pad_id))
            .ok_or_else(|| {
                Error::InvalidSettings(
                    "pad_token_id is missing; set it on ClassifierBuilder or use an exported artifact"
                        .into(),
                )
            })?;
        let mut session_builder = match self.session_builder {
            Some(builder) => builder,
            None => Session::builder()?,
        };
        let session = session_builder.commit_from_file(&self.model_path)?;

        Ok(Classifier {
            tokenizer,
            session,
            pad_token_id,
            max_length: self.max_length,
            stride: self.stride,
            batch_size: self.batch_size,
            threshold: self.threshold,
            aggregation: self.aggregation,
        })
    }
}

/// A loaded tokenizer and ONNX Runtime session.
///
/// `classify` takes `&mut self` because ONNX Runtime sessions are serialized by
/// the `ort` crate. Servers should keep this value behind a mutex, or create one
/// classifier per worker.
pub struct Classifier {
    tokenizer: Tokenizer,
    session: Session,
    pad_token_id: u32,
    max_length: usize,
    stride: usize,
    batch_size: usize,
    threshold: f32,
    aggregation: Aggregation,
}

impl Classifier {
    /// Start configuring a classifier from explicit paths.
    pub fn builder(
        model_path: impl Into<PathBuf>,
        tokenizer_path: impl Into<PathBuf>,
    ) -> ClassifierBuilder {
        ClassifierBuilder::new(model_path, tokenizer_path)
    }

    /// Read `classifier_config.json` and create a builder for an artifact directory.
    ///
    /// This is useful when the caller needs to customize the ONNX Runtime
    /// session before loading a potentially large model.
    pub fn builder_from_dir(directory: impl AsRef<Path>) -> Result<ClassifierBuilder, Error> {
        let directory = directory.as_ref();
        let config_path = directory.join(CONFIG_FILE);
        let bytes = fs::read(&config_path).map_err(|source| Error::Read {
            path: config_path.clone(),
            source,
        })?;
        let config: ArtifactConfig =
            serde_json::from_slice(&bytes).map_err(|source| Error::Config {
                path: config_path,
                source,
            })?;

        Ok(Self::builder(
            directory.join(config.model_file),
            directory.join(config.tokenizer_file),
        )
        .pad_token_id(config.pad_token_id)
        .max_length(config.max_length)
        .stride(config.stride)
        .batch_size(config.batch_size)
        .threshold(config.threshold)
        .aggregation(config.aggregation))
    }

    /// Load `classifier_config.json`, the tokenizer, and the model from an artifact directory.
    pub fn from_dir(directory: impl AsRef<Path>) -> Result<Self, Error> {
        Self::builder_from_dir(directory)?.build()
    }

    /// Normalize, tokenize, window, and classify one source file.
    pub fn classify(&mut self, code: &str) -> Result<Classification, Error> {
        let normalized = normalize_code(code)?;
        let encoding = self
            .tokenizer
            .encode(normalized, false)
            .map_err(|error| Error::Encode(error.to_string()))?;
        let chunks = token_windows(encoding.get_ids(), self.max_length, self.stride);
        if chunks.is_empty() {
            return Err(Error::EmptyInput);
        }

        let mut chunk_scores = Vec::with_capacity(chunks.len());
        for batch in chunks.chunks(self.batch_size) {
            let sequence_length = padded_sequence_length(batch);
            let mut input_ids = vec![i64::from(self.pad_token_id); batch.len() * sequence_length];
            let mut attention_mask = vec![0_i64; batch.len() * sequence_length];

            for (row, ids) in batch.iter().enumerate() {
                let offset = row * sequence_length;
                for (column, id) in ids.iter().enumerate() {
                    input_ids[offset + column] = i64::from(*id);
                    attention_mask[offset + column] = 1;
                }
            }

            let input_ids = Tensor::from_array(([batch.len(), sequence_length], input_ids))?;
            let attention_mask =
                Tensor::from_array(([batch.len(), sequence_length], attention_mask))?;
            let outputs = self.session.run(ort::inputs! {
                "input_ids" => input_ids,
                "attention_mask" => attention_mask,
            })?;
            let logits =
                if let Some(logits) = outputs.get("logits").or_else(|| outputs.get("output")) {
                    logits
                } else if outputs.len() > 0 {
                    &outputs[0]
                } else {
                    return Err(Error::InvalidOutput("model returned no outputs".into()));
                };
            let (shape, values) = logits.try_extract_tensor::<f32>()?;
            if values.len() != batch.len() * LOGITS_PER_ITEM {
                return Err(Error::InvalidOutput(format!(
                    "expected logits shaped [{}, 2], got {shape:?}",
                    batch.len()
                )));
            }
            chunk_scores.extend(
                values
                    .chunks_exact(2)
                    .map(|pair| softmax_ai(pair[0], pair[1])),
            );
        }

        let ai_probability = match self.aggregation {
            Aggregation::Mean => chunk_scores.iter().sum::<f32>() / chunk_scores.len() as f32,
            Aggregation::Max => chunk_scores
                .iter()
                .copied()
                .fold(f32::NEG_INFINITY, f32::max),
        };
        let prediction = if ai_probability >= self.threshold {
            Prediction::Ai
        } else {
            Prediction::Human
        };

        Ok(Classification {
            prediction,
            ai_probability,
            human_probability: 1.0 - ai_probability,
            chunks: chunk_scores.len(),
            chunk_ai_probabilities: chunk_scores,
            aggregation: self.aggregation,
        })
    }
}

fn validate_settings(
    max_length: usize,
    stride: usize,
    batch_size: usize,
    threshold: f32,
) -> Result<(), Error> {
    if max_length < 2 {
        return Err(Error::InvalidSettings(
            "max_length must be at least 2".into(),
        ));
    }
    if stride >= max_length {
        return Err(Error::InvalidSettings(
            "stride must be smaller than max_length".into(),
        ));
    }
    if batch_size == 0 {
        return Err(Error::InvalidSettings(
            "batch_size must be at least 1".into(),
        ));
    }
    if !(0.0..=1.0).contains(&threshold) || !threshold.is_finite() {
        return Err(Error::InvalidSettings(
            "threshold must be a finite number between 0 and 1".into(),
        ));
    }
    Ok(())
}

fn normalize_code(code: &str) -> Result<String, Error> {
    if code.contains('\0') {
        return Err(Error::EmptyInput);
    }
    // Python decodes request bytes as `utf-8-sig`, which removes one leading BOM.
    let code = code.strip_prefix('\u{feff}').unwrap_or(code);
    let normalized: String = code.nfc().collect();
    let normalized = normalized.replace("\r\n", "\n").replace('\r', "\n");
    let mut lines: Vec<&str> = normalized
        .split('\n')
        .map(|line| line.trim_end_matches([' ', '\t']))
        .collect();
    while lines.first().is_some_and(|line| line.is_empty()) {
        lines.remove(0);
    }
    while lines.last().is_some_and(|line| line.is_empty()) {
        lines.pop();
    }
    let normalized = if lines.is_empty() {
        String::new()
    } else {
        format!("{}\n", lines.join("\n"))
    };
    if normalized.trim().is_empty() {
        Err(Error::EmptyInput)
    } else {
        Ok(normalized)
    }
}

fn token_windows(ids: &[u32], max_length: usize, stride: usize) -> Vec<&[u32]> {
    if ids.is_empty() {
        return Vec::new();
    }
    let step = max_length - stride;
    let mut chunks = Vec::with_capacity(ids.len().div_ceil(step));
    let mut start = 0;
    loop {
        let end = (start + max_length).min(ids.len());
        chunks.push(&ids[start..end]);
        if end == ids.len() {
            break;
        }
        start += step;
    }
    chunks
}

fn padded_sequence_length(batch: &[&[u32]]) -> usize {
    let longest = batch.iter().map(|ids| ids.len()).max().unwrap_or(1);
    longest.div_ceil(8) * 8
}

fn softmax_ai(human_logit: f32, ai_logit: f32) -> f32 {
    let maximum = human_logit.max(ai_logit);
    let human = (human_logit - maximum).exp();
    let ai = (ai_logit - maximum).exp();
    ai / (human + ai)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalization_matches_python_pipeline() {
        let code = "\u{feff}\r\n  cafe\u{301}  \r\nnext\t\r\n\r\n";
        assert_eq!(normalize_code(code).unwrap(), "  café\nnext\n");
    }

    #[test]
    fn normalization_rejects_empty_and_nul() {
        assert!(matches!(normalize_code("  \n\t"), Err(Error::EmptyInput)));
        assert!(matches!(
            normalize_code("fn x() {}\0"),
            Err(Error::EmptyInput)
        ));
    }

    #[test]
    fn windows_overlap_by_stride() {
        let ids: Vec<u32> = (0..10).collect();
        let windows = token_windows(&ids, 4, 1);
        assert_eq!(windows, vec![&ids[0..4], &ids[3..7], &ids[6..10]]);
    }

    #[test]
    fn softmax_is_stable() {
        assert!((softmax_ai(1001.0, 1000.0) - 0.268_941_43).abs() < 1e-6);
    }
}
