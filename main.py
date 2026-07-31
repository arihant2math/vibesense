"""Flask server for code-authorship inference.

Send a JSON request such as ``{"code": "print('hello')"}`` to ``POST /file``.
The model is loaded on the first request and reused for later requests.
"""

from __future__ import annotations

import os
import threading
from pathlib import Path
from typing import Any

import torch
from flask import Flask, current_app, jsonify, request
from peft import AutoPeftModelForSequenceClassification
from transformers import AutoTokenizer, DataCollatorWithPadding

from classifier.prepare_dataset import normalize_code
from classifier.run import (
    DEFAULT_MODEL_DIR,
    classify_text,
    resolve_device,
    resolve_precision,
    training_window_settings,
)


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


def create_app(config: dict[str, Any] | None = None) -> Flask:
    app = Flask(__name__)
    app.config.from_mapping(
        MODEL_DIR=os.getenv("VIBESENSE_MODEL_DIR", str(DEFAULT_MODEL_DIR)),
        MODEL_DEVICE=os.getenv("VIBESENSE_DEVICE", "auto"),
        MODEL_PRECISION=os.getenv("VIBESENSE_PRECISION", "auto"),
        MODEL_BATCH_SIZE=int(os.getenv("VIBESENSE_BATCH_SIZE", "1")),
        MODEL_THRESHOLD=float(os.getenv("VIBESENSE_THRESHOLD", "0.5")),
        MODEL_AGGREGATION=os.getenv("VIBESENSE_AGGREGATION", "mean"),
    )
    if config:
        app.config.update(config)

    load_lock = threading.Lock()

    def inference_service() -> InferenceService:
        # Tests and deployments can inject a compatible preloaded service.
        configured_service = current_app.config.get("INFERENCE_SERVICE")
        if configured_service is not None:
            return configured_service

        service = current_app.extensions.get("vibesense_inference")
        if service is None:
            with load_lock:
                service = current_app.extensions.get("vibesense_inference")
                if service is None:
                    service = InferenceService(
                        model_dir=current_app.config["MODEL_DIR"],
                        device=current_app.config["MODEL_DEVICE"],
                        precision=current_app.config["MODEL_PRECISION"],
                        batch_size=current_app.config["MODEL_BATCH_SIZE"],
                        threshold=current_app.config["MODEL_THRESHOLD"],
                        aggregation=current_app.config["MODEL_AGGREGATION"],
                    )
                    current_app.extensions["vibesense_inference"] = service
        return service

    @app.post("/file")
    def classify_file():
        if not request.is_json:
            return jsonify(error="Content-Type must be application/json"), 415

        payload = request.get_json(silent=True)
        if isinstance(payload, str):
            code = payload
            name = "<request>"
        elif isinstance(payload, dict):
            code = next(
                (
                    payload[key]
                    for key in ("code", "content", "text", "data", "file")
                    if key in payload
                ),
                None,
            )
            name = payload.get("name", "<request>")
        else:
            return jsonify(error="Request body must be a JSON object"), 400

        if not isinstance(code, str):
            return jsonify(error="JSON field 'code' must be a string"), 400
        if not isinstance(name, str):
            return jsonify(error="JSON field 'name' must be a string"), 400

        try:
            result = inference_service().classify(code, name)
        except ValueError as error:
            return jsonify(error=str(error)), 400

        return jsonify(
            prediction=result["prediction"],
            ai_probability=result["ai_probability"],
            human_probability=result["human_probability"],
        )

    return app


app = create_app()


def main() -> None:
    app.run(
        host=os.getenv("HOST", "0.0.0.0"),
        port=int(os.getenv("PORT", "5000")),
    )


if __name__ == "__main__":
    main()
