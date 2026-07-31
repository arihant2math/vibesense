"""Fine-tune a LoRA code-authorship classifier on the prepared JSONL data."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any

import numpy as np
import torch
from peft import LoraConfig, TaskType, get_peft_model
from torch.utils.data import Dataset
from transformers import (
    AutoModelForSequenceClassification,
    AutoTokenizer,
    DataCollatorWithPadding,
    EvalPrediction,
    Trainer,
    TrainingArguments,
    set_seed,
)

PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DATA_DIR = Path(__file__).resolve().parent / "data" / "processed"


class CodeDataset(Dataset):
    """A small in-memory view over prepared JSONL records."""

    def __init__(
        self,
        path: Path,
        tokenizer: Any,
        max_length: int,
        max_samples: int | None = None,
    ) -> None:
        if not path.is_file():
            raise FileNotFoundError(
                f"Dataset split not found: {path}. Run classifier/prepare_dataset.py first."
            )

        self.examples: list[tuple[str, int]] = []
        with path.open(encoding="utf-8") as input_file:
            for line_number, line in enumerate(input_file, start=1):
                if not line.strip():
                    continue
                record = json.loads(line)
                text = record.get("text")
                label = record.get("label")
                if not isinstance(text, str) or label not in (0, 1):
                    raise ValueError(f"Invalid record in {path}:{line_number}")
                self.examples.append((text, int(label)))

        if max_samples is not None:
            self.examples = self.examples[:max_samples]
        if not self.examples:
            raise ValueError(f"Dataset split is empty: {path}")

        self.tokenizer = tokenizer
        self.max_length = max_length

    def __len__(self) -> int:
        return len(self.examples)

    def __getitem__(self, index: int) -> dict[str, Any]:
        text, label = self.examples[index]
        encoded = self.tokenizer(
            text,
            max_length=self.max_length,
            truncation=True,
            padding=False,
        )
        encoded["labels"] = label
        return encoded


def binary_auroc(labels: np.ndarray, scores: np.ndarray) -> float:
    """Compute ROC-AUC with average ranks for tied scores."""
    positive_count = int(labels.sum())
    negative_count = int(len(labels) - positive_count)
    if positive_count == 0 or negative_count == 0:
        return float("nan")

    order = np.argsort(scores, kind="stable")
    sorted_scores = scores[order]
    ranks = np.empty(len(scores), dtype=np.float64)
    start = 0
    while start < len(scores):
        end = start + 1
        while end < len(scores) and sorted_scores[end] == sorted_scores[start]:
            end += 1
        # Average of the one-indexed ranks [start + 1, end].
        ranks[order[start:end]] = (start + end + 1) / 2
        start = end

    positive_rank_sum = ranks[labels == 1].sum()
    return float(
        (positive_rank_sum - positive_count * (positive_count + 1) / 2)
        / (positive_count * negative_count)
    )


def average_precision(labels: np.ndarray, scores: np.ndarray) -> float:
    """Compute threshold-based average precision, grouping tied scores."""
    positive_count = int(labels.sum())
    if positive_count == 0:
        return float("nan")

    order = np.argsort(-scores, kind="stable")
    sorted_labels = labels[order]
    sorted_scores = scores[order]
    true_positives = np.cumsum(sorted_labels)
    false_positives = np.cumsum(1 - sorted_labels)
    threshold_ends = np.r_[np.flatnonzero(np.diff(sorted_scores)), len(scores) - 1]
    true_positives = true_positives[threshold_ends]
    false_positives = false_positives[threshold_ends]
    recall = true_positives / positive_count
    precision = true_positives / (true_positives + false_positives)
    recall_increase = np.diff(np.r_[0.0, recall])
    return float(np.sum(recall_increase * precision))


def compute_metrics(prediction: EvalPrediction) -> dict[str, float]:
    raw_predictions = prediction.predictions
    if isinstance(raw_predictions, tuple):
        raw_predictions = raw_predictions[0]
    logits = np.asarray(raw_predictions)
    labels = np.asarray(prediction.label_ids, dtype=np.int64)

    shifted = logits - logits.max(axis=1, keepdims=True)
    probabilities = np.exp(shifted)
    probabilities /= probabilities.sum(axis=1, keepdims=True)
    ai_scores = probabilities[:, 1]
    predicted = (ai_scores >= 0.5).astype(np.int64)

    true_positive = int(np.sum((predicted == 1) & (labels == 1)))
    false_positive = int(np.sum((predicted == 1) & (labels == 0)))
    false_negative = int(np.sum((predicted == 0) & (labels == 1)))
    true_negative = int(np.sum((predicted == 0) & (labels == 0)))

    precision = true_positive / max(true_positive + false_positive, 1)
    recall = true_positive / max(true_positive + false_negative, 1)
    f1 = 2 * precision * recall / max(precision + recall, 1e-12)
    accuracy = (true_positive + true_negative) / len(labels)

    return {
        "accuracy": float(accuracy),
        "precision": float(precision),
        "recall": float(recall),
        "f1": float(f1),
        "auroc": binary_auroc(labels, ai_scores),
        "average_precision": average_precision(labels, ai_scores),
    }


def mps_supports_bf16() -> bool:
    if not torch.backends.mps.is_available():
        return False
    try:
        torch.ones(1, device="mps", dtype=torch.bfloat16)
    except (RuntimeError, TypeError):
        return False
    return True


def choose_precision(requested: str, use_cpu: bool) -> str:
    if requested != "auto":
        return requested
    if use_cpu:
        return "fp32"
    if torch.cuda.is_available():
        return "bf16" if torch.cuda.is_bf16_supported() else "fp16"
    if mps_supports_bf16():
        return "bf16"
    return "fp32"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="Qwen/Qwen2.5-Coder-1.5B")
    parser.add_argument("--data-dir", type=Path, default=DEFAULT_DATA_DIR)
    parser.add_argument("--output-dir", type=Path, default=PROJECT_ROOT / "detector")
    parser.add_argument("--max-length", type=int, default=1024)
    parser.add_argument("--epochs", type=float, default=2.0)
    parser.add_argument("--learning-rate", type=float, default=2e-4)
    parser.add_argument("--batch-size", type=int, default=1)
    parser.add_argument("--eval-batch-size", type=int, default=1)
    parser.add_argument("--gradient-accumulation-steps", type=int, default=8)
    parser.add_argument("--lora-r", type=int, default=16)
    parser.add_argument("--lora-alpha", type=int, default=32)
    parser.add_argument("--lora-dropout", type=float, default=0.05)
    parser.add_argument("--precision", choices=("auto", "bf16", "fp16", "fp32"), default="auto")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--cpu", action="store_true", help="Force CPU training")
    parser.add_argument("--max-train-samples", type=int)
    parser.add_argument("--max-eval-samples", type=int)
    parser.add_argument(
        "--resume-from-checkpoint",
        nargs="?",
        const=True,
        default=None,
        help="Resume from the latest checkpoint, or optionally provide its path",
    )
    return parser.parse_args()


def main() -> None:
    cli_args = parse_args()
    set_seed(cli_args.seed)

    precision = choose_precision(cli_args.precision, cli_args.cpu)
    if precision == "bf16" and not cli_args.cpu:
        supported = torch.cuda.is_available() and torch.cuda.is_bf16_supported()
        supported = supported or mps_supports_bf16()
        if not supported:
            raise RuntimeError("bf16 was requested but is not supported by the active accelerator")

    tokenizer = AutoTokenizer.from_pretrained(cli_args.model)
    if tokenizer.pad_token_id is None:
        if tokenizer.eos_token_id is None:
            raise RuntimeError("Tokenizer has neither a padding token nor an EOS token")
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"

    train_dataset = CodeDataset(
        cli_args.data_dir / "train.jsonl",
        tokenizer,
        cli_args.max_length,
        cli_args.max_train_samples,
    )
    validation_dataset = CodeDataset(
        cli_args.data_dir / "validation.jsonl",
        tokenizer,
        cli_args.max_length,
        cli_args.max_eval_samples,
    )
    test_dataset = CodeDataset(
        cli_args.data_dir / "test.jsonl",
        tokenizer,
        cli_args.max_length,
        cli_args.max_eval_samples,
    )

    dtype = {
        "bf16": torch.bfloat16,
        "fp16": torch.float16,
        "fp32": torch.float32,
    }[precision]
    model = AutoModelForSequenceClassification.from_pretrained(
        cli_args.model,
        num_labels=2,
        id2label={0: "human", 1: "ai"},
        label2id={"human": 0, "ai": 1},
        dtype=dtype,
    )
    model.config.pad_token_id = tokenizer.pad_token_id
    model.config.use_cache = False

    lora_config = LoraConfig(
        task_type=TaskType.SEQ_CLS,
        r=cli_args.lora_r,
        lora_alpha=cli_args.lora_alpha,
        lora_dropout=cli_args.lora_dropout,
        target_modules=["q_proj", "v_proj"],
        bias="none",
    )
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()

    update_steps_per_epoch = math.ceil(
        len(train_dataset)
        / (cli_args.batch_size * cli_args.gradient_accumulation_steps)
    )
    warmup_steps = max(1, round(update_steps_per_epoch * cli_args.epochs * 0.1))

    training_args = TrainingArguments(
        output_dir=str(cli_args.output_dir),
        learning_rate=cli_args.learning_rate,
        per_device_train_batch_size=cli_args.batch_size,
        per_device_eval_batch_size=cli_args.eval_batch_size,
        gradient_accumulation_steps=cli_args.gradient_accumulation_steps,
        num_train_epochs=cli_args.epochs,
        warmup_steps=warmup_steps,
        weight_decay=0.01,
        eval_strategy="epoch",
        save_strategy="epoch",
        logging_strategy="steps",
        logging_steps=10,
        load_best_model_at_end=True,
        metric_for_best_model="auroc",
        greater_is_better=True,
        save_total_limit=2,
        bf16=precision == "bf16",
        fp16=precision == "fp16",
        gradient_checkpointing=True,
        dataloader_pin_memory=torch.cuda.is_available(),
        report_to="none",
        seed=cli_args.seed,
        data_seed=cli_args.seed,
        use_cpu=cli_args.cpu,
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=validation_dataset,
        data_collator=DataCollatorWithPadding(tokenizer, pad_to_multiple_of=8),
        processing_class=tokenizer,
        compute_metrics=compute_metrics,
    )

    print(
        f"Training on {len(train_dataset):,} records; validating on "
        f"{len(validation_dataset):,}; precision={precision}; max_length={cli_args.max_length}"
    )
    train_result = trainer.train(resume_from_checkpoint=cli_args.resume_from_checkpoint)
    trainer.save_model()
    tokenizer.save_pretrained(cli_args.output_dir)
    trainer.log_metrics("train", train_result.metrics)
    trainer.save_metrics("train", train_result.metrics)
    trainer.save_state()

    test_metrics = trainer.evaluate(test_dataset, metric_key_prefix="test")
    trainer.log_metrics("test", test_metrics)
    trainer.save_metrics("test", test_metrics)

    run_config = {
        **vars(cli_args),
        "data_dir": str(cli_args.data_dir),
        "output_dir": str(cli_args.output_dir),
        "resolved_precision": precision,
        "train_records": len(train_dataset),
        "validation_records": len(validation_dataset),
        "test_records": len(test_dataset),
    }
    cli_args.output_dir.mkdir(parents=True, exist_ok=True)
    (cli_args.output_dir / "run_config.json").write_text(
        json.dumps(run_config, indent=2, default=str) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
