# Code classifier

## Train

Install the dependencies and prepare the dataset:

```bash
uv sync
uv run python classifier/prepare_dataset.py
```

Smoke test:

```bash
uv run python classifier/train.py \
  --max-train-samples 32 \
  --max-eval-samples 32 \
  --max-length 256 \
  --epochs 1 \
  --output-dir detector-smoke
```

Run:

```bash
uv run python classifier/train.py
```

If the process runs out of memory, shorten the context:

```bash
uv run python classifier/train.py --max-length 512
```

Resume an interrupted run from the newest checkpoint in the output directory:

```bash
uv run python classifier/train.py --resume-from-checkpoint
```

Or provide a specific checkpoint:

```bash
uv run python classifier/train.py \
  --resume-from-checkpoint detector/checkpoint-100
```

## Outputs

The default output directory is `detector/`. It contains the LoRA adapter and classification head, tokenizer files, checkpoints, training state, and these result files:

- `train_results.json`
- `test_results.json`
- `run_config.json`

Validation selects the best checkpoint by ROC-AUC. Final test output also reports accuracy, precision, recall, F1, ROC-AUC, and average precision.

The saved artifact is a PEFT adapter referencing the Qwen base model, rather than a second full copy of Qwen.

## Interpretation warning

The current dataset has only one repository per label. High test scores can therefore mean the model learned Servo versus TrustOS conventions rather than human versus AI authorship. Add multiple repositories and generators per class, then evaluate using entirely held-out repositories, before using the score outside experimentation.
