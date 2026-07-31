"""Convert the prepared splits into the prompt/completion format mlx-lm LoRA expects.

mlx-lm reads {train,valid,test}.jsonl from a directory and, for prompt/completion records,
wraps them in the model's chat template. The completion is the single token `ai` or
`human`, so with --mask-prompt the loss lands on the authorship decision rather than on the
code itself.

Code is truncated here rather than by the trainer: mlx-lm truncates from the end, which
would cut off the answer token and silently destroy the label.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from transformers import AutoTokenizer

DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_INPUT = DATA_DIR / "processed"
DEFAULT_OUTPUT = DATA_DIR / "mlx"
DEFAULT_MODEL = "Qwen/Qwen2.5-Coder-1.5B"

# mlx-lm names the evaluation split "valid".
SPLIT_FILENAMES = {"train": "train", "validation": "valid", "test": "test"}

# Chat template wrapper plus the completion tokens; reserved so nothing is cut off.
TEMPLATE_OVERHEAD_TOKENS = 48


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument(
        "--max-seq-length",
        type=int,
        default=1024,
        help="Total sequence budget; must match the value passed to mlx_lm lora",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    tokenizer = AutoTokenizer.from_pretrained(args.model)
    budget = args.max_seq_length - TEMPLATE_OVERHEAD_TOKENS
    if budget < 32:
        raise SystemExit("error: --max-seq-length leaves no room for code")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    for split, filename in SPLIT_FILENAMES.items():
        source = args.input_dir / f"{split}.jsonl"
        if not source.is_file():
            print(f"skipping {split}: {source} does not exist")
            continue

        written = 0
        truncated = 0
        destination = args.output_dir / f"{filename}.jsonl"
        with source.open(encoding="utf-8") as handle, destination.open(
            "w", encoding="utf-8"
        ) as output:
            for line in handle:
                record = json.loads(line)
                ids = tokenizer.encode(record["text"], add_special_tokens=False)
                if len(ids) > budget:
                    ids = ids[:budget]
                    truncated += 1
                output.write(
                    json.dumps(
                        {
                            "prompt": tokenizer.decode(ids),
                            "completion": record["label_name"],
                        },
                        ensure_ascii=False,
                    )
                    + "\n"
                )
                written += 1
        print(f"{filename}.jsonl: {written:,} records ({truncated:,} truncated to {budget} tokens)")

    print(f"\nwrote {args.output_dir}")


if __name__ == "__main__":
    main()
