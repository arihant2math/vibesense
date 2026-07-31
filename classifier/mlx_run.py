"""Score code with an mlx-lm LoRA adapter trained to answer `ai` or `human`.

The model is asked to produce one token. Rather than sampling, the logits at that position
are restricted to the two label tokens and softmaxed, which yields a calibrated probability
without ever generating text.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Any

import mlx.core as mx
from mlx_lm import load

try:
    from .prepare_dataset import iter_code_files, normalize_code
except ImportError:  # Support `python classifier/mlx_run.py`.
    from prepare_dataset import iter_code_files, normalize_code

PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ADAPTER = PROJECT_ROOT / "detector-mlx"
DEFAULT_MODEL = "Qwen/Qwen2.5-Coder-1.5B"
LABEL_TOKENS = ("human", "ai")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", type=Path, nargs="*", help="Files or directories to classify")
    parser.add_argument("--code", "--text", dest="code", help="Classify code supplied directly")
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--adapter-path", type=Path, default=DEFAULT_ADAPTER)
    parser.add_argument("--no-adapter", action="store_true", help="Score with the base model")
    parser.add_argument("--max-length", type=int, default=1024)
    parser.add_argument("--stride", type=int, default=None)
    parser.add_argument("--threshold", type=float, default=0.5)
    parser.add_argument("--aggregation", choices=("mean", "max"), default="mean")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--include-chunks", action="store_true")
    parser.add_argument(
        "--eval",
        dest="eval_split",
        type=Path,
        help="Score a prepared split (jsonl with text/label) and report AUROC",
    )
    parser.add_argument("--limit", type=int, help="Evaluate only the first N records")
    args = parser.parse_args()
    if args.code is not None and args.paths:
        parser.error("--code cannot be combined with file paths")
    if not 0 <= args.threshold <= 1:
        parser.error("--threshold must be between 0 and 1")
    return args


class Scorer:
    """Wraps the model and the two-token readout."""

    def __init__(self, model_name: str, adapter_path: Path | None, max_length: int):
        self.model, self.tokenizer = load(
            model_name, adapter_path=str(adapter_path) if adapter_path else None
        )
        self.max_length = max_length
        self.label_ids = []
        for label in LABEL_TOKENS:
            ids = self.tokenizer.encode(label, add_special_tokens=False)
            if len(ids) != 1:
                raise RuntimeError(f"label {label!r} is not a single token: {ids}")
            self.label_ids.append(ids[0])

    def prompt_ids(self, code: str) -> list[int]:
        """Build the same chat-template prompt mlx_prepare.py trains against."""
        return self.tokenizer.apply_chat_template(
            [{"role": "user", "content": code}],
            add_generation_prompt=True,
        )

    def score_text(self, code: str) -> float:
        ids = self.prompt_ids(code)
        if len(ids) > self.max_length:
            # Keep the tail: it holds the generation prompt the answer follows.
            ids = ids[: self.max_length - 16] + ids[-16:]
        logits = self.model(mx.array([ids]))[0, -1]
        pair = mx.softmax(mx.array([logits[self.label_ids[0]], logits[self.label_ids[1]]]))
        return float(pair[1])

    def token_chunks(self, code: str, stride: int) -> list[str]:
        """Split long code into overlapping windows that each fit the prompt budget."""
        budget = self.max_length - 48
        ids = self.tokenizer.encode(code, add_special_tokens=False)
        if len(ids) <= budget:
            return [code]
        chunks = []
        start = 0
        while start < len(ids):
            chunks.append(self.tokenizer.decode(ids[start : start + budget]))
            if start + budget >= len(ids):
                break
            start += budget - stride
        return chunks


def auroc(scores: list[float], labels: list[int]) -> float:
    pairs = sorted(zip(scores, labels))
    ranks: dict[int, float] = {}
    index = 0
    while index < len(pairs):
        end = index
        while end + 1 < len(pairs) and pairs[end + 1][0] == pairs[index][0]:
            end += 1
        average = (index + end) / 2 + 1
        for position in range(index, end + 1):
            ranks[position] = average
        index = end + 1

    positives = sum(1 for _, label in pairs if label == 1)
    negatives = len(pairs) - positives
    if not positives or not negatives:
        return float("nan")
    rank_sum = sum(ranks[position] for position, (_, label) in enumerate(pairs) if label == 1)
    return (rank_sum - positives * (positives + 1) / 2) / (positives * negatives)


def evaluate(scorer: Scorer, path: Path, args: argparse.Namespace) -> None:
    records = [json.loads(line) for line in path.open(encoding="utf-8")]
    if args.limit:
        records = records[: args.limit]

    scores: list[float] = []
    labels: list[int] = []
    started = time.perf_counter()
    for index, record in enumerate(records, start=1):
        scores.append(scorer.score_text(record["text"]))
        labels.append(record["label"])
        if index % 200 == 0:
            rate = index / (time.perf_counter() - started)
            print(f"  {index}/{len(records)} ({rate:.1f}/s)", flush=True)

    area = auroc(scores, labels)
    predictions = [(score >= args.threshold) == (label == 1) for score, label in zip(scores, labels)]
    accuracy = sum(predictions) / len(predictions)
    positives = sum(labels)
    mean_ai = sum(s for s, l in zip(scores, labels) if l == 1) / max(positives, 1)
    mean_human = sum(s for s, l in zip(scores, labels) if l == 0) / max(len(labels) - positives, 1)

    print(
        f"\n{path.name}: {len(records):,} chunks ({positives:,} ai)\n"
        f"  AUROC     {area:.4f}\n"
        f"  accuracy  {accuracy:.4f} at threshold {args.threshold}\n"
        f"  mean p(ai) given ai={mean_ai:.3f} human={mean_human:.3f}"
    )


def normalized_file(path: Path) -> str:
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
        normalized = normalize_code(sys.stdin.buffer.read())
        if normalized is None or not normalized.strip():
            raise ValueError("stdin is empty, binary, or not valid UTF-8")
        return [("<stdin>", normalized)]

    inputs: list[tuple[str, str]] = []
    for path in args.paths:
        if path.is_dir():
            for code_path in iter_code_files(path):
                inputs.append((str(code_path), normalized_file(code_path)))
        else:
            inputs.append((str(path), normalized_file(path)))
    if not inputs:
        raise ValueError("No recognized code files were found")
    return inputs


def main() -> None:
    args = parse_args()
    adapter = None if args.no_adapter else args.adapter_path
    if adapter is not None and not (adapter / "adapters.safetensors").is_file():
        raise SystemExit(
            f"error: no adapter in {adapter}. Train one with classifier/mlx_train.py, "
            "or pass --no-adapter to score with the base model."
        )

    scorer = Scorer(args.model, adapter, args.max_length)
    stride = args.stride if args.stride is not None else min(128, args.max_length // 4)

    if args.eval_split:
        evaluate(scorer, args.eval_split, args)
        return

    results: list[dict[str, Any]] = []
    for name, text in collect_inputs(args):
        chunk_scores = [scorer.score_text(chunk) for chunk in scorer.token_chunks(text, stride)]
        probability = (
            max(chunk_scores)
            if args.aggregation == "max"
            else sum(chunk_scores) / len(chunk_scores)
        )
        result = {
            "input": name,
            "prediction": "ai" if probability >= args.threshold else "human",
            "ai_probability": probability,
            "human_probability": 1.0 - probability,
            "threshold": args.threshold,
            "chunks": len(chunk_scores),
            "aggregation": args.aggregation,
        }
        if args.include_chunks:
            result["chunk_ai_probabilities"] = chunk_scores
        results.append(result)

    if args.json:
        print(json.dumps(results[0] if len(results) == 1 else results, indent=2))
        return

    for index, result in enumerate(results):
        if index:
            print()
        print(f"Input: {result['input']}")
        print(f"Prediction: {result['prediction']}")
        print(f"AI probability: {result['ai_probability']:.2%}")
        print(f"Chunks: {result['chunks']} ({result['aggregation']})")


if __name__ == "__main__":
    main()
