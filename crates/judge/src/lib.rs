//! LLM-assisted repository inspection.
//!
//! The judge can only see the repository through an [`access::Accessor`]. It
//! repeatedly services file and directory tool calls until the model returns a
//! structured [`JudgeOutput`].

use access::Accessor;
use genai::{
    Client, ModelSpec,
    adapter::AdapterKind,
    chat::{ChatMessage, ChatOptions, ChatRequest, JsonSpec, Tool, ToolCall, ToolResponse},
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};

pub const DEFAULT_MODEL: &str = "gemini/gemini-3.1-pro-preview";

pub const SYSTEM_PROMPT: &str = "You are part of a pipeline meant to detect AI generated code.\n\
Return false for both is_ai and is_human if you are not sure.\n\
List some key files in `key_files`. This entails files that are part of the core logic of the system\n\
(i.e. not machine generated, vendored, or config files).\n\
These will be fed into a classifier which will perform spot testing on these files.\n\
\n\
Add any relevant comments in comments.\n";

/// The repository-level assessment returned by the judge model.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub struct JudgeOutput {
    pub is_ai: bool,
    pub is_human: bool,
    pub key_files: Vec<String>,
    pub comments: String,
}

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("judge model request failed: {0}")]
    Model(#[from] genai::Error),

    #[error("judge returned neither tool calls nor an answer")]
    MissingAnswer,

    #[error("judge returned invalid structured output: {source}; response was {response:?}")]
    InvalidOutput {
        response: String,
        #[source]
        source: serde_json::Error,
    },
}

pub type Result<T> = std::result::Result<T, Error>;

/// Inspect a repository with [`DEFAULT_MODEL`] and credentials from the
/// environment (`GEMINI_API_KEY`, `OPENAI_API_KEY`, and so on).
pub async fn check_repo(accessor: &(impl Accessor + ?Sized)) -> Result<JudgeOutput> {
    check_repo_with_model(accessor, DEFAULT_MODEL).await
}

/// Inspect a repository with an explicit model name.
///
/// A LiteLLM-style `gemini/` prefix is accepted and removed before dispatching
/// through the Gemini adapter. Other model names use `genai`'s normal model
/// resolution.
pub async fn check_repo_with_model(
    accessor: &(impl Accessor + ?Sized),
    model: &str,
) -> Result<JudgeOutput> {
    let client = Client::default();
    check_repo_with_client(accessor, &client, model).await
}

/// Inspect a repository using a preconfigured `genai` client.
///
/// This entry point allows callers to provide custom credentials, endpoint
/// mappings, or default chat options.
pub async fn check_repo_with_client(
    accessor: &(impl Accessor + ?Sized),
    client: &Client,
    model: &str,
) -> Result<JudgeOutput> {
    let mut request = ChatRequest::from_user(
        "Inspect the repository using the available tools and determine whether its code is AI generated.",
    )
    .with_system(SYSTEM_PROMPT)
    .with_tools(repository_tools());
    let options = ChatOptions::default()
        .with_response_format(JsonSpec::new("judge_output", judge_output_schema()));
    let model = resolve_model(model);

    loop {
        let response = client
            .exec_chat(model.clone(), request.clone(), Some(&options))
            .await?;
        let tool_calls: Vec<ToolCall> = response.tool_calls().into_iter().cloned().collect();

        if tool_calls.is_empty() {
            let content = response
                .content
                .joined_texts()
                .ok_or(Error::MissingAnswer)?;
            return serde_json::from_str(&content).map_err(|source| Error::InvalidOutput {
                response: content,
                source,
            });
        }

        // Preserve the complete assistant turn, including provider-specific
        // thought signatures required when continuing Gemini tool calls.
        request = request.append_message(ChatMessage::assistant(response.content));
        let responses = tool_calls
            .iter()
            .map(|tool_call| {
                ToolResponse::from_tool_call(tool_call, execute_tool(accessor, tool_call))
            })
            .collect::<Vec<_>>();
        request = request.append_message(responses);
    }
}

fn resolve_model(model: &str) -> ModelSpec {
    if let Some(model) = model.strip_prefix("gemini/") {
        ModelSpec::from_iden((AdapterKind::Gemini, model))
    } else {
        ModelSpec::from_name(model)
    }
}

fn repository_tools() -> Vec<Tool> {
    vec![
        Tool::new("tool_read_file")
            .with_description("Read all or part of a UTF-8 text file in the repository.")
            .with_schema(json!({
                "type": "object",
                "properties": {
                    "relative_path": {
                        "type": "string",
                        "description": "Path to the file, relative to the repository root."
                    },
                    "offset": {
                        "type": "integer",
                        "minimum": 0,
                        "description": "Optional character offset at which to start reading."
                    },
                    "limit": {
                        "type": "integer",
                        "minimum": 0,
                        "description": "Optional maximum number of characters to return."
                    }
                },
                "required": ["relative_path"]
            })),
        Tool::new("tool_list_directory")
            .with_description("List the files and directories at a repository path.")
            .with_schema(json!({
                "type": "object",
                "properties": {
                    "relative_path": {
                        "type": "string",
                        "description": "Directory path relative to the repository root; use '.' for the root."
                    }
                },
                "required": ["relative_path"]
            })),
    ]
}

fn judge_output_schema() -> Value {
    json!({
        "type": "object",
        "properties": {
            "is_ai": { "type": "boolean" },
            "is_human": { "type": "boolean" },
            "key_files": {
                "type": "array",
                "items": { "type": "string" }
            },
            "comments": { "type": "string" }
        },
        "required": ["is_ai", "is_human", "key_files", "comments"],
        "additionalProperties": false
    })
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct ReadFileArguments {
    relative_path: String,
    offset: Option<usize>,
    limit: Option<usize>,
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct ListDirectoryArguments {
    relative_path: String,
}

fn execute_tool(accessor: &(impl Accessor + ?Sized), tool_call: &ToolCall) -> String {
    let result = match tool_call.fn_name.as_str() {
        "tool_read_file" => {
            serde_json::from_value::<ReadFileArguments>(tool_call.fn_arguments.clone())
                .map_err(|error| format!("invalid tool arguments: {error}"))
                .and_then(|arguments| {
                    accessor
                        .read_file(&arguments.relative_path, arguments.offset, arguments.limit)
                        .map_err(|error| error.to_string())
                })
        }
        "tool_list_directory" => {
            serde_json::from_value::<ListDirectoryArguments>(tool_call.fn_arguments.clone())
                .map_err(|error| format!("invalid tool arguments: {error}"))
                .and_then(|arguments| {
                    accessor
                        .list_dir(&arguments.relative_path)
                        .map_err(|error| error.to_string())
                        .and_then(|entries| {
                            serde_json::to_string(&entries).map_err(|error| {
                                format!("could not serialize directory listing: {error}")
                            })
                        })
                })
        }
        name => Err(format!("unknown tool: {name}")),
    };

    match result {
        Ok(content) => content,
        Err(error) => json!({ "error": error }).to_string(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use access::{DirEntry, Error as AccessError, Result as AccessResult};

    struct TestAccessor;

    impl Accessor for TestAccessor {
        fn list_dir(&self, relative_path: &str) -> AccessResult<Vec<DirEntry>> {
            if relative_path == "." {
                Ok(vec![DirEntry::file("lib.rs", Some(12))])
            } else {
                Err(AccessError::NotFound(relative_path.into()))
            }
        }

        fn read_file(
            &self,
            relative_path: &str,
            offset: Option<usize>,
            limit: Option<usize>,
        ) -> AccessResult<String> {
            if relative_path != "lib.rs" {
                return Err(AccessError::NotFound(relative_path.into()));
            }
            let text = "abcdef";
            Ok(text
                .chars()
                .skip(offset.unwrap_or(0))
                .take(limit.unwrap_or(usize::MAX))
                .collect())
        }
    }

    fn tool_call(name: &str, arguments: Value) -> ToolCall {
        ToolCall {
            call_id: "call-1".into(),
            fn_name: name.into(),
            fn_arguments: arguments,
            thought_signatures: None,
        }
    }

    #[test]
    fn executes_read_and_list_tools() {
        let read = tool_call(
            "tool_read_file",
            json!({ "relative_path": "lib.rs", "offset": 1, "limit": 3 }),
        );
        assert_eq!(execute_tool(&TestAccessor, &read), "bcd");

        let list = tool_call("tool_list_directory", json!({ "relative_path": "." }));
        assert_eq!(
            execute_tool(&TestAccessor, &list),
            r#"[{"name":"lib.rs","entry_type":"file","size":12}]"#
        );
    }

    #[test]
    fn tool_failures_are_returned_to_the_model_as_json() {
        let unknown = tool_call("delete_repository", json!({}));
        let value: Value = serde_json::from_str(&execute_tool(&TestAccessor, &unknown)).unwrap();
        assert_eq!(value["error"], "unknown tool: delete_repository");

        let bad_offset = tool_call(
            "tool_read_file",
            json!({ "relative_path": "lib.rs", "offset": -1 }),
        );
        let value: Value = serde_json::from_str(&execute_tool(&TestAccessor, &bad_offset)).unwrap();
        assert!(
            value["error"]
                .as_str()
                .unwrap()
                .contains("invalid tool arguments")
        );
    }

    #[test]
    fn default_model_uses_the_gemini_adapter() {
        assert!(matches!(resolve_model(DEFAULT_MODEL), ModelSpec::Iden(_)));
    }
}
