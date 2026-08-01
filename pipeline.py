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
        classification = service.classify(code, file)
        if classification['ai_probability'] > certain_threshold:
            ai.append(classification)
        if classification['ai_probability'] > classification['threshold']:
            prob_ai.append(classification)

    print(f"{len(ai)} out of {len(result.key_files)} key files are certainly AI generated", end="")
    if len(ai) - len(prob_ai) > 0:
        print(", another {len(ai) - len(prob_ai)} are probably AI generated or assisted in some way")
    else:
        print()
    print("Worst offenders")
    for file in sorted(ai, key=lambda result: result["ai_probability"]):
        print(f"- {file["input"]}: {file["ai_probability"]}")

