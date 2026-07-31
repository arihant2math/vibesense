# Code classifier

## Add a Git source

Add a repository at its current `HEAD`, labeled as human (`0`) or AI (`1`):

```bash
uv run classifier/add_git_source.py <git-url> <0-or-1>
```

The command pins the resolved commit in `classifier/data/sources.json`. Use `--id <source-id>` if the repository name conflicts with an existing source.

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

## Inference

Classify a file using the adapter in `detector/`:

```bash
uv run python classifier/run.py path/to/source.py
```

Classify code from stdin or directly on the command line:

```bash
cat path/to/source.rs | uv run python classifier/run.py

uv run python classifier/run.py --code 'def add(a, b): return a + b'
```

Directories are scanned recursively using the same code-file filtering and normalization as dataset preparation:

```bash
uv run python classifier/run.py path/to/repository
```

For machine-readable output:

```bash
uv run python classifier/run.py path/to/source.py --json
```

Long files are classified in overlapping token chunks and use their mean AI probability. Use `--aggregation max` to flag a file when any chunk scores highly, or `--include-chunks` to display each score. The default decision threshold is `0.5` and can be changed with `--threshold`.

Use another trained adapter with `--model-dir detector-smoke`. The base Qwen model referenced by the adapter is downloaded automatically if it is not already cached.

## Interpretation warning

The current dataset has only one repository per label. High test scores can therefore mean the model learned Servo versus TrustOS conventions rather than human versus AI authorship. Add multiple repositories and generators per class, then evaluate using entirely held-out repositories, before using the score outside experimentation.
