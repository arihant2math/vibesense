#!/usr/bin/env python3
"""Merge the Vibesense PEFT adapter and export a Rust-ready ONNX artifact.

This is intentionally an offline build step. The Rust crate never needs Python,
PyTorch, Transformers, or PEFT at runtime.
"""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

import torch
from peft import AutoPeftModelForSequenceClassification
from transformers import AutoTokenizer


class LogitsOnly(torch.nn.Module):
    """Give the exported graph a single, stable float32 output."""

    def __init__(self, model: torch.nn.Module) -> None:
        super().__init__()
        self.model = model

    def forward(
        self, input_ids: torch.Tensor, attention_mask: torch.Tensor
    ) -> torch.Tensor:
        return self.model(
            input_ids=input_ids,
            attention_mask=attention_mask,
            use_cache=False,
            return_dict=False,
        )[0].float()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adapter-dir", type=Path, default=Path("detector"))
    parser.add_argument("--output-dir", type=Path, default=Path("detector-onnx"))
    parser.add_argument("--precision", choices=("fp32", "fp16"), default="fp32")
    parser.add_argument("--opset", type=int, default=18)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    adapter_dir = args.adapter_dir.expanduser().resolve()
    output_dir = args.output_dir.expanduser().resolve()
    if not (adapter_dir / "adapter_config.json").is_file():
        raise FileNotFoundError(f"No PEFT adapter found in {adapter_dir}")

    # torch.onnx's legacy exporter gives a particularly stable graph for this
    # decoder classifier, but it imports the standalone onnx package.
    try:
        import onnx  # noqa: F401
    except ImportError as error:
        raise SystemExit(
            "The exporter needs onnx. Run it with: "
            "uv run --with onnx python crates/classifier/export_onnx.py"
        ) from error

    run_config_path = adapter_dir / "run_config.json"
    run_config = (
        json.loads(run_config_path.read_text(encoding="utf-8"))
        if run_config_path.is_file()
        else {}
    )
    max_length = int(run_config.get("max_length", 1024))
    stride = int(run_config.get("stride", min(128, max_length // 4)))
    dtype = torch.float32 if args.precision == "fp32" else torch.float16

    print(f"Loading and merging adapter from {adapter_dir} ({args.precision})...")
    model = AutoPeftModelForSequenceClassification.from_pretrained(
        adapter_dir,
        is_trainable=False,
        dtype=dtype,
        device_map="cpu",
        attn_implementation="eager",
    )
    model = model.merge_and_unload(safe_merge=True)
    model.config.use_cache = False
    model.eval()

    tokenizer = AutoTokenizer.from_pretrained(adapter_dir)
    if tokenizer.pad_token_id is None:
        if tokenizer.eos_token_id is None:
            raise RuntimeError("Tokenizer has neither a padding nor an EOS token")
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"

    # External tensors are emitted as many files whose names are chosen by
    # PyTorch. Recreate the directory so a re-export cannot leave stale weights.
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True)
    tokenizer.save_pretrained(output_dir)

    # A short input is enough to trace the graph; both dimensions are dynamic.
    input_ids = torch.tensor([[1, 2, 3, 4, 5, 6, 7, 8]], dtype=torch.int64)
    attention_mask = torch.ones_like(input_ids)
    model_path = output_dir / "model.onnx"
    print(f"Exporting {model_path}; large weights will be stored as external data...")
    with torch.inference_mode():
        torch.onnx.export(
            LogitsOnly(model),
            (input_ids, attention_mask),
            str(model_path),  # Legacy exporter requires `str` for >2 GiB external data.
            input_names=["input_ids", "attention_mask"],
            output_names=["logits"],
            dynamic_axes={
                "input_ids": {0: "batch", 1: "sequence"},
                "attention_mask": {0: "batch", 1: "sequence"},
                "logits": {0: "batch"},
            },
            opset_version=args.opset,
            dynamo=False,
            external_data=True,
            do_constant_folding=True,
        )

    config = {
        "model_file": model_path.name,
        "tokenizer_file": "tokenizer.json",
        "pad_token_id": tokenizer.pad_token_id,
        "max_length": max_length,
        "stride": stride,
        "batch_size": 1,
        "threshold": 0.5,
        "aggregation": "mean",
    }
    (output_dir / "classifier_config.json").write_text(
        json.dumps(config, indent=2) + "\n", encoding="utf-8"
    )
    # The Rust runtime does not consume these files. Remove extra files emitted
    # by save_pretrained so the artifact contract remains small and obvious.
    for extra in ("chat_template.jinja",):
        path = output_dir / extra
        if path.exists():
            path.unlink()

    size_gib = sum(path.stat().st_size for path in output_dir.iterdir()) / 1024**3
    print(f"Wrote {output_dir} ({size_gib:.2f} GiB)")
    print("Run: cargo run -p vibesense-classifier --example classify_ai_code")


if __name__ == "__main__":
    main()
