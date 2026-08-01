"""Flask server for code-authorship inference.

Send a JSON request such as ``{"code": "print('hello')"}`` to ``POST /file``.
The model is loaded on the first request and reused for later requests.
"""

from __future__ import annotations

import os
import threading
from typing import Any

from flask import Flask, current_app, jsonify, request

from classifier.run import (
    DEFAULT_MODEL_DIR,
)
from inference_service import InferenceService


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
