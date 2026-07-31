"""Train the authorship LoRA with mlx-lm on Apple Silicon.

This wraps `mlx_lm lora` with defaults suited to the task: prompt masking so the loss lands
only on the answer token, bf16 (the fastest dtype on the M5 neural accelerators), and a
sequence budget matched to what mlx_prepare.py wrote.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_DATA = DATA_DIR / "mlx"
DEFAULT_ADAPTER = PROJECT_ROOT / "detector-mlx"
DEFAULT_MODEL = "Qwen/Qwen2.5-Coder-1.5B"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--data", type=Path, default=DEFAULT_DATA)
    parser.add_argument("--adapter-path", type=Path, default=DEFAULT_ADAPTER)
    parser.add_argument("--iters", type=int, default=1000)
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--num-layers", type=int, default=16, help="-1 fine-tunes every layer")
    parser.add_argument("--learning-rate", type=float, default=1e-5)
    parser.add_argument("--max-seq-length", type=int, default=1024)
    parser.add_argument("--steps-per-eval", type=int, default=100)
    parser.add_argument("--save-every", type=int, default=200)
    parser.add_argument("--val-batches", type=int, default=50)
    parser.add_argument("--grad-checkpoint", action="store_true", help="Trade speed for memory")
    parser.add_argument("--resume", action="store_true", help="Continue from the saved adapter")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--dry-run", action="store_true", help="Print the command and exit")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    train_file = args.data / "train.jsonl"
    if not train_file.is_file():
        raise SystemExit(
            f"error: {train_file} does not exist. Run classifier/mlx_prepare.py first."
        )

    command = [
        sys.executable, "-m", "mlx_lm", "lora",
        "--model", args.model,
        "--train",
        "--data", str(args.data),
        "--fine-tune-type", "lora",
        # The label is one token; without this the loss is dominated by the code itself.
        "--mask-prompt",
        "--num-layers", str(args.num_layers),
        "--batch-size", str(args.batch_size),
        "--iters", str(args.iters),
        "--learning-rate", str(args.learning_rate),
        "--max-seq-length", str(args.max_seq_length),
        "--steps-per-eval", str(args.steps_per_eval),
        "--save-every", str(args.save_every),
        "--val-batches", str(args.val_batches),
        "--adapter-path", str(args.adapter_path),
        "--seed", str(args.seed),
    ]
    if args.grad_checkpoint:
        command.append("--grad-checkpoint")
    if args.resume:
        command += ["--resume-adapter-file", str(args.adapter_path / "adapters.safetensors")]

    print(" ".join(command), flush=True)
    if args.dry_run:
        return

    args.adapter_path.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(command, check=False)
    if result.returncode != 0:
        raise SystemExit(result.returncode)

    print(
        f"\nAdapter written to {args.adapter_path}\n"
        f"Evaluate with:\n"
        f"  uv run python classifier/mlx_run.py --eval classifier/data/processed/test.jsonl"
    )


if __name__ == "__main__":
    main()
