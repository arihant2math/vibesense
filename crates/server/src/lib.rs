//! JSON HTTP interface for Vibesense.
//!
//! The server exposes file-level classification at `POST /code` and sampled
//! GitHub repository analysis at `POST /github`. A loaded detector is shared
//! by both endpoints and inference is kept off Tokio executor threads.

use std::sync::{Arc, Mutex, PoisonError};

use access::{Accessor, GitHubAccessor};
use axum::{
    Json, Router,
    extract::{DefaultBodyLimit, FromRef, State, rejection::JsonRejection},
    http::StatusCode,
    response::{IntoResponse, Response},
    routing::{get, post},
};
use pipeline::{
    Classification, ClassifierError, Config as PipelineConfig, FileClassifier, RepoStats, analyze,
};
use serde::{Deserialize, Serialize};
use vibesense_classifier::Classifier;

/// Maximum JSON request size accepted by the API.
pub const MAX_REQUEST_BYTES: usize = 2 * 1024 * 1024;

/// A classifier that can be shared by HTTP requests.
///
/// Implementing this trait separately from the ONNX-backed implementation
/// keeps the router easy to embed and test.
pub trait CodeDetector: Send + Sync + 'static {
    fn classify(&self, code: &str) -> Result<Classification, ClassifierError>;
}

/// Thread-safe wrapper around one loaded ONNX classifier.
pub struct OnnxDetector {
    classifier: Mutex<Classifier>,
}

impl OnnxDetector {
    pub fn new(classifier: Classifier) -> Self {
        Self {
            classifier: Mutex::new(classifier),
        }
    }

    /// Load an exported Vibesense artifact directory.
    pub fn from_dir(directory: impl AsRef<std::path::Path>) -> Result<Self, ClassifierError> {
        Classifier::from_dir(directory).map(Self::new)
    }
}

impl CodeDetector for OnnxDetector {
    fn classify(&self, code: &str) -> Result<Classification, ClassifierError> {
        self.classifier
            .lock()
            .unwrap_or_else(PoisonError::into_inner)
            .classify(code)
    }
}

#[derive(Clone)]
struct AppState {
    detector: Arc<dyn CodeDetector>,
    pipeline_config: PipelineConfig,
}

impl FromRef<AppState> for Arc<dyn CodeDetector> {
    fn from_ref(state: &AppState) -> Self {
        state.detector.clone()
    }
}

/// Build the API router with the default repository-analysis configuration.
pub fn app(detector: Arc<dyn CodeDetector>) -> Router {
    app_with_config(detector, PipelineConfig::default())
}

/// Build the API router with an operator-supplied repository configuration.
///
/// Pipeline limits are deliberately configured by the server operator rather
/// than accepted from untrusted `/github` request bodies.
pub fn app_with_config(detector: Arc<dyn CodeDetector>, pipeline_config: PipelineConfig) -> Router {
    let state = AppState {
        detector,
        pipeline_config,
    };

    Router::new()
        .route("/health", get(health))
        .route("/code", post(classify_code))
        .route("/github", post(classify_github))
        .fallback(not_found)
        .method_not_allowed_fallback(method_not_allowed)
        .layer(DefaultBodyLimit::max(MAX_REQUEST_BYTES))
        .with_state(state)
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
struct CodeRequest {
    code: String,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
struct GitHubRequest {
    /// An `owner/repository` pair or a GitHub HTTPS, SSH, or scp-style URL.
    repository: String,
    /// Optional branch, tag, or commit SHA.
    #[serde(default, rename = "ref")]
    git_ref: Option<String>,
}

#[derive(Serialize)]
struct HealthResponse {
    status: &'static str,
}

async fn health() -> Json<HealthResponse> {
    Json(HealthResponse { status: "ok" })
}

async fn classify_code(
    State(detector): State<Arc<dyn CodeDetector>>,
    payload: Result<Json<CodeRequest>, JsonRejection>,
) -> Result<Json<Classification>, ApiError> {
    let Json(request) = payload.map_err(ApiError::invalid_json)?;
    let result = tokio::task::spawn_blocking(move || detector.classify(&request.code))
        .await
        .map_err(|error| ApiError::internal("classification_task_failed", error.to_string()))?
        .map_err(ApiError::classifier)?;
    Ok(Json(result))
}

async fn classify_github(
    State(state): State<AppState>,
    payload: Result<Json<GitHubRequest>, JsonRejection>,
) -> Result<Json<RepoStats>, ApiError> {
    let Json(request) = payload.map_err(ApiError::invalid_json)?;
    let mut accessor = GitHubAccessor::new(&request.repository).map_err(ApiError::access)?;
    if let Some(git_ref) = request.git_ref {
        accessor = accessor.with_ref(git_ref).map_err(|error| {
            ApiError::new(StatusCode::BAD_REQUEST, "invalid_ref", error.to_string())
        })?;
    }

    let accessor: Arc<dyn Accessor> = Arc::new(accessor);
    let classifier = SharedFileClassifier(state.detector);
    let result = analyze(accessor, classifier, state.pipeline_config)
        .await
        .map_err(ApiError::pipeline)?;
    Ok(Json(result))
}

/// Adapter allowing the pipeline to use any shared HTTP detector.
struct SharedFileClassifier(Arc<dyn CodeDetector>);

impl FileClassifier for SharedFileClassifier {
    fn classify_file(&mut self, code: &str) -> Result<Classification, ClassifierError> {
        self.0.classify(code)
    }
}

#[derive(Debug)]
struct ApiError {
    status: StatusCode,
    code: &'static str,
    message: String,
}

impl ApiError {
    fn new(status: StatusCode, code: &'static str, message: impl Into<String>) -> Self {
        Self {
            status,
            code,
            message: message.into(),
        }
    }

    fn invalid_json(error: JsonRejection) -> Self {
        Self::new(error.status(), "invalid_json", error.body_text())
    }

    fn internal(code: &'static str, message: impl Into<String>) -> Self {
        Self::new(StatusCode::INTERNAL_SERVER_ERROR, code, message)
    }

    fn classifier(error: ClassifierError) -> Self {
        match error {
            ClassifierError::EmptyInput => {
                Self::new(StatusCode::BAD_REQUEST, "invalid_code", error.to_string())
            }
            error => Self::internal("classification_failed", error.to_string()),
        }
    }

    fn access(error: access::Error) -> Self {
        let (status, code) = match &error {
            access::Error::InvalidPath { .. } | access::Error::InvalidRepository { .. } => {
                (StatusCode::BAD_REQUEST, "invalid_repository")
            }
            access::Error::PermissionDenied { .. } => {
                (StatusCode::FORBIDDEN, "github_permission_denied")
            }
            access::Error::NotFound(_) => (StatusCode::NOT_FOUND, "repository_not_found"),
            access::Error::GitHub { status, .. } => (
                StatusCode::from_u16(status.as_u16()).unwrap_or(StatusCode::BAD_GATEWAY),
                "github_error",
            ),
            access::Error::Http(_) | access::Error::Metadata(_) => {
                (StatusCode::BAD_GATEWAY, "github_unavailable")
            }
            access::Error::Io { .. } | access::Error::Utf8 { .. } => (
                StatusCode::INTERNAL_SERVER_ERROR,
                "repository_access_failed",
            ),
        };
        Self::new(status, code, error.to_string())
    }

    fn pipeline(error: pipeline::Error) -> Self {
        match error {
            pipeline::Error::RootAccess(error) => Self::access(error),
            pipeline::Error::Worker(error) => {
                Self::internal("analysis_task_failed", error.to_string())
            }
            pipeline::Error::Runtime(error) => {
                Self::internal("analysis_runtime_failed", error.to_string())
            }
        }
    }
}

#[derive(Serialize)]
struct ErrorResponse {
    error: ErrorDetails,
}

#[derive(Serialize)]
struct ErrorDetails {
    code: &'static str,
    message: String,
}

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let body = ErrorResponse {
            error: ErrorDetails {
                code: self.code,
                message: self.message,
            },
        };
        (self.status, Json(body)).into_response()
    }
}

async fn not_found() -> ApiError {
    ApiError::new(StatusCode::NOT_FOUND, "not_found", "endpoint not found")
}

async fn method_not_allowed() -> ApiError {
    ApiError::new(
        StatusCode::METHOD_NOT_ALLOWED,
        "method_not_allowed",
        "method not allowed for this endpoint",
    )
}

#[cfg(test)]
mod tests {
    use axum::{
        body::{Body, to_bytes},
        http::{Request, header},
    };
    use pipeline::{Classification, ClassifierError};
    use serde_json::{Value, json};
    use tower::ServiceExt;
    use vibesense_classifier::{Aggregation, Prediction};

    use super::*;

    struct FakeDetector;

    impl CodeDetector for FakeDetector {
        fn classify(&self, code: &str) -> Result<Classification, ClassifierError> {
            if code.trim().is_empty() {
                return Err(ClassifierError::EmptyInput);
            }
            Ok(Classification {
                prediction: Prediction::Ai,
                ai_probability: 0.75,
                human_probability: 0.25,
                chunks: 1,
                chunk_ai_probabilities: vec![0.75],
                aggregation: Aggregation::Mean,
            })
        }
    }

    fn test_app() -> Router {
        app(Arc::new(FakeDetector))
    }

    async fn json_body(response: Response) -> Value {
        let bytes = to_bytes(response.into_body(), MAX_REQUEST_BYTES)
            .await
            .unwrap();
        serde_json::from_slice(&bytes).unwrap()
    }

    #[tokio::test]
    async fn code_endpoint_returns_a_json_classification() {
        let response = test_app()
            .oneshot(
                Request::post("/code")
                    .header(header::CONTENT_TYPE, "application/json")
                    .body(Body::from(json!({ "code": "fn main() {}" }).to_string()))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);
        let body = json_body(response).await;
        assert_eq!(body["prediction"], "ai");
        assert_eq!(body["ai_probability"], 0.75);
        assert_eq!(body["chunks"], 1);
    }

    #[tokio::test]
    async fn empty_code_is_a_json_client_error() {
        let response = test_app()
            .oneshot(
                Request::post("/code")
                    .header(header::CONTENT_TYPE, "application/json")
                    .body(Body::from(r#"{"code":"   "}"#))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        assert_eq!(json_body(response).await["error"]["code"], "invalid_code");
    }

    #[tokio::test]
    async fn malformed_payloads_are_json_errors() {
        let response = test_app()
            .oneshot(
                Request::post("/code")
                    .header(header::CONTENT_TYPE, "application/json")
                    .body(Body::from(r#"{"text":"wrong field"}"#))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::UNPROCESSABLE_ENTITY);
        assert_eq!(json_body(response).await["error"]["code"], "invalid_json");
    }

    #[tokio::test]
    async fn github_rejects_invalid_repositories_before_network_access() {
        let response = test_app()
            .oneshot(
                Request::post("/github")
                    .header(header::CONTENT_TYPE, "application/json")
                    .body(Body::from(
                        json!({ "repository": "not-a-repo" }).to_string(),
                    ))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::BAD_REQUEST);
        assert_eq!(
            json_body(response).await["error"]["code"],
            "invalid_repository"
        );
    }

    #[tokio::test]
    async fn routing_errors_are_json() {
        let response = test_app()
            .oneshot(Request::get("/missing").body(Body::empty()).unwrap())
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::NOT_FOUND);
        assert_eq!(json_body(response).await["error"]["code"], "not_found");

        let response = test_app()
            .oneshot(Request::get("/code").body(Body::empty()).unwrap())
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::METHOD_NOT_ALLOWED);
        assert_eq!(
            json_body(response).await["error"]["code"],
            "method_not_allowed"
        );
    }
}
