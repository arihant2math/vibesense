from access import Accessor
from inference_service import InferenceService
from judge import check_repo


def run_pipeline(accessor: Accessor, service: InferenceService | None = None) -> None:
    """Inspect and classify the repository exposed by ``accessor``."""
    if service is None:
        service = InferenceService()
    result = check_repo(accessor)
    print(result.model_dump_json(indent=2))

    certain_ai = []
    probable_ai = []
    certain_threshold = 0.75
    for file in result.key_files:
        code = accessor.read_file(file)
        classification = service.classify(code, file)
        if classification["ai_probability"] > certain_threshold:
            certain_ai.append(classification)
        elif classification["ai_probability"] > classification["threshold"]:
            probable_ai.append(classification)

    print(
        f"{len(certain_ai)} out of {len(result.key_files)} key files are "
        "certainly AI generated",
        end="",
    )
    if probable_ai:
        print(
            f", another {len(probable_ai)} are probably AI generated or assisted "
            "in some way"
        )
    else:
        print()

    print("Worst offenders")
    offenders = sorted(
        certain_ai + probable_ai,
        key=lambda classification: classification["ai_probability"],
        reverse=True,
    )
    for classification in offenders:
        print(f"- {classification['input']}: {classification['ai_probability']}")
