from __future__ import annotations

import threading
from pathlib import Path
from typing import Any

import torch
from peft import AutoPeftModelForSequenceClassification
from transformers import AutoTokenizer, DataCollatorWithPadding

from classifier.prepare_dataset import normalize_code
from classifier.run import DEFAULT_MODEL_DIR, resolve_device, resolve_precision, training_window_settings, classify_text


class InferenceService:
    """Load the detector once and provide thread-safe inference."""

    def __init__(
        self,
        model_dir: str | Path = DEFAULT_MODEL_DIR,
        device: str = "auto",
        precision: str = "auto",
        batch_size: int = 1,
        threshold: float = 0.5,
        aggregation: str = "mean",
    ) -> None:
        self.model_dir = Path(model_dir).expanduser().resolve()
        if not (self.model_dir / "adapter_config.json").is_file():
            raise FileNotFoundError(f"No trained adapter found in {self.model_dir}")
        if batch_size < 1:
            raise ValueError("batch_size must be at least 1")
        if not 0 <= threshold <= 1:
            raise ValueError("threshold must be between 0 and 1")
        if aggregation not in {"mean", "max"}:
            raise ValueError("aggregation must be 'mean' or 'max'")

        self.device = resolve_device(device)
        resolved_precision = resolve_precision(precision, self.device)
        dtype = {
            "bf16": torch.bfloat16,
            "fp16": torch.float16,
            "fp32": torch.float32,
        }[resolved_precision]
        self.max_length, self.stride = training_window_settings(self.model_dir)
        self.batch_size = batch_size
        self.threshold = threshold
        self.aggregation = aggregation

        self.tokenizer = AutoTokenizer.from_pretrained(self.model_dir)
        if self.tokenizer.pad_token_id is None:
            if self.tokenizer.eos_token_id is None:
                raise RuntimeError("Tokenizer has neither a padding token nor an EOS token")
            self.tokenizer.pad_token = self.tokenizer.eos_token
        self.tokenizer.padding_side = "right"

        self.model = AutoPeftModelForSequenceClassification.from_pretrained(
            self.model_dir,
            is_trainable=False,
            dtype=dtype,
        )
        self.model.config.pad_token_id = self.tokenizer.pad_token_id
        self.model.config.id2label = {0: "human", 1: "ai"}
        self.model.config.label2id = {"human": 0, "ai": 1}
        self.model.to(self.device)
        self.model.eval()

        self.collator = DataCollatorWithPadding(self.tokenizer, pad_to_multiple_of=8)
        self._inference_lock = threading.Lock()

    def classify(self, code: str, name: str = "<request>") -> dict[str, Any]:
        normalized = normalize_code(code.encode("utf-8"))
        if normalized is None or not normalized.strip():
            raise ValueError("code must contain non-empty UTF-8 text without NUL bytes")

        # Serialize access because some accelerators are not safe to invoke from
        # several Flask worker threads at the same time.
        with self._inference_lock:
            return classify_text(
                text=normalized,
                display_name=name,
                model=self.model,
                tokenizer=self.tokenizer,
                collator=self.collator,
                device=self.device,
                max_length=self.max_length,
                stride=self.stride,
                batch_size=self.batch_size,
                threshold=self.threshold,
                aggregation=self.aggregation,
                include_chunks=False,
            )
