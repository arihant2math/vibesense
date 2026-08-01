from pathlib import Path

from access import DirectoryAccessor
from inference_service import InferenceService
from judge import check_repo

if __name__ == "__main__":
    service = InferenceService()
    path = Path("../prontum")
    result = check_repo(DirectoryAccessor(path))
    print(result.model_dump_json(indent=2))
    ai = []
    prob_ai = []
    certain_threshold = 0.75
    for file in result.key_files:
        code = open(path / file).read()
        result = service.classify(code, file)
        if result['ai_probability'] > certain_threshold:
            ai.append(result)
        if result['ai_probability'] > result['threshold']:
            prob_ai.append(result)
    print(f"{len(ai)} out of {len(result.key_files)} key files are certainly AI generated, {len(prob_ai)} are probably AI generated or assisted in some way")
    print("Worst offenders")
    for file in sorted(ai, key=lambda result: result["ai_probability"]):
        print(f"- {file["input"]}: {file["ai_probability"]}")

