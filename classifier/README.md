# Code classifier

## Train

Install the dependencies and prepare the dataset:

```bash
uv sync
uv run python classifier/prepare_dataset.py
```

Sources hosted on GitHub are fetched as pinned `tar.gz` archives, read straight out of the
archive without unpacking, and cached by commit under `data/.cache/repos/archives`. Use
`--jobs` to change how many sources are fetched concurrently (default 8), and
`--no-archives` to force the original Git clone path. Setting `GITHUB_TOKEN` raises the API
rate limit from 60 to 5,000 requests per hour.

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

## Inference

Classify a file using the adapter in `detector/`:

```bash
uv run python classifier/run.py path/to/source.py
```

For machine-readable output:

```bash
uv run python classifier/run.py path/to/source.py --json
```

Long files are classified in overlapping token chunks and use their mean AI probability. Use `--aggregation max` to flag a file when any chunk scores highly, or `--include-chunks` to display each score. The default decision threshold is `0.5` and can be changed with `--threshold`.
