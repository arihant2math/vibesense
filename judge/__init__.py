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
    response = completion(
        model="gemini/gemini-3.1-pro-preview",
        messages=[
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": "Hello, how are you?"},
        ],
        response_format=JudgeOutput
    )
    print(response)

check_repo(DirectoryAccessor("../winbond_flash_driver"))
