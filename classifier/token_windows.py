"""Shared token-window helpers for training and inference."""

from __future__ import annotations

from typing import Any


def token_window_capacity(tokenizer: Any, max_length: int) -> int:
    """Return the number of content tokens available in a model input."""
    if max_length < 2:
        raise ValueError("max_length must be at least 2")
    capacity = max_length - tokenizer.num_special_tokens_to_add(pair=False)
    if capacity < 1:
        raise ValueError("max_length leaves no room for content tokens")
    return capacity


def token_chunks(
    tokenizer: Any,
    text: str,
    max_length: int,
    stride: int,
) -> list[dict[str, list[int]]]:
    """Tokenize text into overlapping, model-ready windows."""
    capacity = token_window_capacity(tokenizer, max_length)
    if stride < 0:
        raise ValueError("stride cannot be negative")
    if stride >= capacity:
        raise ValueError("stride must be smaller than the content-token capacity")

    encoded = tokenizer(
        text,
        truncation=True,
        max_length=max_length,
        stride=stride,
        return_overflowing_tokens=True,
        padding=False,
    )
    input_ids = encoded["input_ids"]
    if input_ids and isinstance(input_ids[0], int):
        input_ids = [input_ids]

    chunks: list[dict[str, list[int]]] = []
    for index, ids in enumerate(input_ids):
        feature = {"input_ids": ids}
        for name in tokenizer.model_input_names:
            values = encoded.get(name)
            if name == "input_ids" or values is None:
                continue
            if values and isinstance(values[0], int):
                values = [values]
            feature[name] = values[index]
        chunks.append(feature)
    return chunks
