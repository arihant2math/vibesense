from pathlib import Path

from access import DirectoryAccessor
from inference_service import InferenceService
from judge import check_repo

if __name__ == "__main__":
    service = InferenceService()
    path = Path("../prontum")
    result = check_repo(DirectoryAccessor(path))
    print(result.model_dump_json(indent=2))
    for file in result.key_files:
        code = open(path / file).read()
        print(service.classify(code, file))

