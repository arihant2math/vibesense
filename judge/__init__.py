from litellm import completion

from access import Accessor

SYSTEM_PROMPT = "You are part of a pipeline meant to detect AI generated code."

def check_repo(accessor: Accessor):
  response = completion(
    model="gemini/gemini-3.1-pro-preview",
    messages=[{"role": "system", "content": SYSTEM_PROMPT}, {"role": "user", "content": "Hello, how are you?"}]
  )

  print(response)
