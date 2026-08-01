import json
from typing import Any, Optional

from litellm import completion
from pydantic import BaseModel

from access import Accessor, DirectoryAccessor

SYSTEM_PROMPT = """You are part of a pipeline meant to detect AI generated code.
Return false for both is_ai and is_human if you are not sure.
List some key files in `key_files`. This entails files that are part of the core logic of the system
(i.e. not machine generated, vendored, or config files).
These will be fed into a classifier which will perform spot testing on these files.

Add any relevant comments in comments.
"""


class JudgeOutput(BaseModel):
    is_ai: bool
    is_human: bool
    key_files: list[str]
    comments: str


def check_repo(accessor: Accessor, model="gemini/gemini-3.1-pro-preview") -> JudgeOutput:
    def tool_read_file(
        relative_path: str,
        offset: Optional[int] = None,
        limit: Optional[int] = None,
    ):
        return accessor.read_file(relative_path, offset, limit)

    def tool_list_directory(relative_path: str):
        return [
            {
                "name": entry.name,
                "entry_type": entry.entry_type.value,
                "size": entry.size,
            }
            for entry in accessor.list_dir(relative_path)
        ]

    tool_functions = {
        "tool_read_file": tool_read_file,
        "tool_list_directory": tool_list_directory,
    }

    tools = [
        {
            "type": "function",
            "function": {
                "name": "tool_read_file",
                "description": "Read all or part of a UTF-8 text file in the repository.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "relative_path": {
                            "type": "string",
                            "description": "Path to the file, relative to the repository root.",
                        },
                        "offset": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "Optional character offset at which to start reading.",
                        },
                        "limit": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "Optional maximum number of characters to return.",
                        },
                    },
                    "required": ["relative_path"],
                },
            },
        },
        {
            "type": "function",
            "function": {
                "name": "tool_list_directory",
                "description": "List the files and directories at a repository path.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "relative_path": {
                            "type": "string",
                            "description": (
                                "Directory path relative to the repository root; use '.' "
                                "for the root."
                            ),
                        },
                    },
                    "required": ["relative_path"],
                },
            },
        },
    ]

    messages: list[dict[str, Any]] = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {
            "role": "user",
            "content": (
                "Inspect the repository using the available tools and determine "
                "whether its code is AI generated."
            ),
        },
    ]

    while True:
        response = completion(
            model=model,
            messages=messages,
            response_format=JudgeOutput,
            tools=tools,
        )
        message = response.choices[0].message
        tool_calls = message.tool_calls or []

        if not tool_calls:
            if not message.content:
                raise RuntimeError("Judge returned neither tool calls nor an answer")
            return JudgeOutput.model_validate_json(message.content)

        messages.append(message.to_dict(mode="json", exclude_none=True))
        for tool_call in tool_calls:
            function_name = tool_call.function.name
            try:
                arguments = json.loads(tool_call.function.arguments)
                if not isinstance(arguments, dict):
                    raise TypeError("tool arguments must be a JSON object")

                tool_function = tool_functions.get(function_name)
                if tool_function is None:
                    raise ValueError(f"Unknown tool: {function_name}")
                result = tool_function(**arguments)
                content = (
                    result
                    if isinstance(result, str)
                    else json.dumps(result, ensure_ascii=False)
                )
            except Exception as error:
                content = json.dumps(
                    {"error": f"{type(error).__name__}: {error}"},
                    ensure_ascii=False,
                )

            messages.append(
                {
                    "role": "tool",
                    "tool_call_id": tool_call.id,
                    "content": content,
                }
            )


if __name__ == "__main__":
    result = check_repo(DirectoryAccessor("../prontum"))
    print(result.model_dump_json(indent=2))
