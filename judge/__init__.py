from typing import Optional

from litellm import completion
from pydantic import BaseModel

from access import Accessor, DirectoryAccessor

SYSTEM_PROMPT = """You are part of a pipeline meant to detect AI generated code.
Return false for both is_ai and is_human if you are not sure.
Return the files most likely to be AI generated in `suspicious_files`.

Add any relevant comments in comments.
"""


class JudgeOutput(BaseModel):
    is_ai: bool
    is_human: bool
    suspicious_files: list[str]
    comments: str


def check_repo(accessor: Accessor):
    def tool_read_file(
        relative_path: str,
        offset: Optional[int] = None,
        limit: Optional[int] = None,
    ):
        return accessor.read_file(relative_path, offset, limit)

    def tool_list_directory(relative_path: str):
        return accessor.list_dir(relative_path)

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

    response = completion(
        model="gemini/gemini-3.1-pro-preview",
        messages=[
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": "Hello, how are you?"},
        ],
        response_format=JudgeOutput,
        tools=tools,
    )
    print(response)

check_repo(DirectoryAccessor("../winbond_flash_driver"))
