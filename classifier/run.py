"""Run code-authorship inference with a trained PEFT classifier."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import torch
from peft import AutoPeftModelForSequenceClassification
from transformers import AutoTokenizer, DataCollatorWithPadding

try:
    from .prepare_dataset import iter_code_files, normalize_code
    from .token_windows import token_chunks
except ImportError:  # Support `python classifier/run.py`.
    from prepare_dataset import iter_code_files, normalize_code
    from token_windows import token_chunks

PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MODEL_DIR = PROJECT_ROOT / "detector"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "paths",
        type=Path,
        nargs="*",
        help="Code files or directories. Reads stdin when omitted or when path is '-'.",
    )
    parser.add_argument("--code", "--text", dest="code", help="Classify code supplied directly")
    parser.add_argument("--model-dir", type=Path, default=DEFAULT_MODEL_DIR)
    parser.add_argument(
        "--device",
        choices=("auto", "cuda", "mps", "cpu"),
        default="auto",
    )
    parser.add_argument(
        "--precision",
        choices=("auto", "bf16", "fp16", "fp32"),
        default="auto",
    )
    parser.add_argument(
        "--max-length",
        type=int,
        help="Tokens per chunk; defaults to the value recorded during training",
    )
    parser.add_argument(
        "--stride",
        type=int,
        help="Overlapping tokens between chunks; defaults to min(128, max_length / 4)",
    )
    parser.add_argument("--batch-size", type=int, default=1)
    parser.add_argument("--threshold", type=float, default=0.5)
    parser.add_argument("--aggregation", choices=("mean", "max"), default="mean")
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON")
    parser.add_argument(
        "--include-chunks",
        action="store_true",
        help="Include every chunk probability in the result",
    )
    args = parser.parse_args()

    if args.code is not None and args.paths:
        parser.error("--code cannot be combined with file paths")
    if not 0 <= args.threshold <= 1:
        parser.error("--threshold must be between 0 and 1")
    if args.batch_size < 1:
        parser.error("--batch-size must be at least 1")
    if args.max_length is not None and args.max_length < 2:
        parser.error("--max-length must be at least 2")
    if args.stride is not None and args.stride < 0:
        parser.error("--stride cannot be negative")
    return args


def resolve_device(requested: str) -> torch.device:
    if requested == "auto":
        if torch.cuda.is_available():
            return torch.device("cuda")
        if torch.backends.mps.is_available():
            return torch.device("mps")
        return torch.device("cpu")

    if requested == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but is not available")
    if requested == "mps" and not torch.backends.mps.is_available():
        raise RuntimeError("MPS was requested but is not available")
    return torch.device(requested)


def mps_supports_bf16() -> bool:
    if not torch.backends.mps.is_available():
        return False
    try:
        torch.ones(1, device="mps", dtype=torch.bfloat16)
    except (RuntimeError, TypeError):
        return False
    return True


def resolve_precision(requested: str, device: torch.device) -> str:
    if requested == "auto":
        if device.type == "cuda":
            return "bf16" if torch.cuda.is_bf16_supported() else "fp16"
        if device.type == "mps" and mps_supports_bf16():
            return "bf16"
        return "fp32"

    if device.type == "cpu" and requested == "fp16":
        raise RuntimeError("fp16 inference is not supported on CPU; use fp32")
    if requested == "bf16":
        supported = device.type == "cuda" and torch.cuda.is_bf16_supported()
        supported = supported or (device.type == "mps" and mps_supports_bf16())
        supported = supported or device.type == "cpu"
        if not supported:
            raise RuntimeError(f"bf16 is not supported on {device.type}")
    return requested


def training_window_settings(model_dir: Path) -> tuple[int, int]:
    max_length = 1024
    stride: int | None = None
    config_path = model_dir / "run_config.json"
    if config_path.is_file():
        try:
            config = json.loads(config_path.read_text(encoding="utf-8"))
            max_length = int(config.get("max_length", max_length))
            configured_stride = config.get("stride")
            stride = int(configured_stride) if configured_stride is not None else None
        except (OSError, ValueError, TypeError, json.JSONDecodeError):
            max_length = 1024
            stride = None

    if max_length < 2:
        max_length = 1024
    if stride is None or stride < 0:
        stride = min(128, max_length // 4)
    return max_length, stride


def classify_text(
    text: str,
    display_name: str,
    model: Any,
    tokenizer: Any,
    collator: DataCollatorWithPadding,
    device: torch.device,
    max_length: int,
    stride: int,
    batch_size: int,
    threshold: float,
    aggregation: str,
    include_chunks: bool,
) -> dict[str, Any]:
    chunks = token_chunks(tokenizer, text, max_length, stride)
    if not chunks:
        raise ValueError(f"No tokens found in {display_name}")

    chunk_scores: list[float] = []
    with torch.inference_mode():
        for start in range(0, len(chunks), batch_size):
            batch = collator(chunks[start : start + batch_size])
            batch = {
                key: value.to(device) if isinstance(value, torch.Tensor) else value
                for key, value in batch.items()
            }
            logits = model(**batch).logits.float()
            probabilities = torch.softmax(logits, dim=-1)[:, 1]
            chunk_scores.extend(probabilities.cpu().tolist())

    if aggregation == "max":
        ai_probability = max(chunk_scores)
    else:
        ai_probability = sum(chunk_scores) / len(chunk_scores)

    result: dict[str, Any] = {
        "input": display_name,
        "prediction": "ai" if ai_probability >= threshold else "human",
        "ai_probability": ai_probability,
        "human_probability": 1.0 - ai_probability,
        "threshold": threshold,
        "chunks": len(chunk_scores),
        "aggregation": aggregation,
    }
    if include_chunks:
        result["chunk_ai_probabilities"] = chunk_scores
    return result


def normalized_file(path: Path) -> str:
    if not path.is_file():
        raise FileNotFoundError(f"Input file does not exist: {path}")
    text = normalize_code(path.read_bytes())
    if text is None:
        raise ValueError(f"Input is binary or not valid UTF-8: {path}")
    if not text.strip():
        raise ValueError(f"Input is empty: {path}")
    return text


def collect_inputs(args: argparse.Namespace) -> list[tuple[str, str]]:
    if args.code is not None:
        normalized = normalize_code(args.code.encode("utf-8"))
        if normalized is None or not normalized.strip():
            raise ValueError("--code is empty")
        return [("<argument>", normalized)]

    if not args.paths:
        if sys.stdin.isatty():
            raise ValueError("Provide a file, directory, --code, or pipe code over stdin")
        raw = sys.stdin.buffer.read()
        normalized = normalize_code(raw)
        if normalized is None or not normalized.strip():
            raise ValueError("stdin is empty, binary, or not valid UTF-8")
        return [("<stdin>", normalized)]

    inputs: list[tuple[str, str]] = []
    stdin_consumed = False
    for path in args.paths:
        if str(path) == "-":
            if stdin_consumed:
                raise ValueError("stdin may only be specified once")
            stdin_consumed = True
            normalized = normalize_code(sys.stdin.buffer.read())
            if normalized is None or not normalized.strip():
                raise ValueError("stdin is empty, binary, or not valid UTF-8")
            inputs.append(("<stdin>", normalized))
        elif path.is_dir():
            for code_path in iter_code_files(path):
                inputs.append((str(code_path), normalized_file(code_path)))
        else:
            inputs.append((str(path), normalized_file(path)))

    if not inputs:
        raise ValueError("No recognized code files were found")
    return inputs


def print_results(results: list[dict[str, Any]], as_json: bool) -> None:
    if as_json:
        payload: dict[str, Any] | list[dict[str, Any]]
        payload = results[0] if len(results) == 1 else results
        print(json.dumps(payload, indent=2))
        return

    for index, result in enumerate(results):
        if index:
            print()
        print(f"Input: {result['input']}")
        print(f"Prediction: {result['prediction']}")
        print(f"AI probability: {result['ai_probability']:.2%}")
        print(f"Human probability: {result['human_probability']:.2%}")
        print(f"Chunks: {result['chunks']} ({result['aggregation']})")
        if "chunk_ai_probabilities" in result:
            formatted = ", ".join(f"{score:.2%}" for score in result["chunk_ai_probabilities"])
            print(f"Chunk AI probabilities: {formatted}")


def main() -> None:
    args = parse_args()
    model_dir = args.model_dir.resolve()
    if not (model_dir / "adapter_config.json").is_file():
        raise FileNotFoundError(
            f"No trained adapter found in {model_dir}. Run classifier/train.py first."
        )

    device = resolve_device(args.device)
    precision = resolve_precision(args.precision, device)
    dtype = {
        "bf16": torch.bfloat16,
        "fp16": torch.float16,
        "fp32": torch.float32,
    }[precision]
    training_max_length, training_stride = training_window_settings(model_dir)
    max_length = args.max_length or training_max_length
    stride = args.stride if args.stride is not None else training_stride
    if args.max_length is not None and args.stride is None:
        stride = min(128, max_length // 4)
    if stride >= max_length:
        raise ValueError("--stride must be smaller than --max-length")
    inputs = collect_inputs(args)

    tokenizer = AutoTokenizer.from_pretrained(model_dir)
    if tokenizer.pad_token_id is None:
        if tokenizer.eos_token_id is None:
            raise RuntimeError("Tokenizer has neither a padding token nor an EOS token")
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"

    model = AutoPeftModelForSequenceClassification.from_pretrained(
        model_dir,
        is_trainable=False,
        dtype=dtype,
    )
    model.config.pad_token_id = tokenizer.pad_token_id
    model.config.id2label = {0: "human", 1: "ai"}
    model.config.label2id = {"human": 0, "ai": 1}
    model.to(device)
    model.eval()

    collator = DataCollatorWithPadding(tokenizer, pad_to_multiple_of=8)
    results = [
        classify_text(
            text=text,
            display_name=name,
            model=model,
            tokenizer=tokenizer,
            collator=collator,
            device=device,
            max_length=max_length,
            stride=stride,
            batch_size=args.batch_size,
            threshold=args.threshold,
            aggregation=args.aggregation,
            include_chunks=args.include_chunks,
        )
        for name, text in inputs
    ]
    print_results(results, args.json)


if __name__ == "__main__":
    main()
