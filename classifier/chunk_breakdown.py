"""Print human and AI chunk counts grouped by programming language."""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import TextIO

DATA_DIR = Path(__file__).resolve().parent / "data" / "processed"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "dataset",
        nargs="?",
        type=Path,
        default=DATA_DIR / "all.jsonl",
        help="JSONL dataset to inspect (default: classifier/data/processed/all.jsonl)",
    )
    return parser.parse_args()


def count_chunks(dataset: Path) -> dict[str, dict[int, int]]:
    counts: dict[str, dict[int, int]] = defaultdict(lambda: {0: 0, 1: 0})

    with dataset.open(encoding="utf-8") as records:
        for line_number, line in enumerate(records, start=1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
                language = record["language"]
                label = record["label"]
            except (json.JSONDecodeError, KeyError, TypeError) as error:
                raise ValueError(
                    f"Invalid record at {dataset}:{line_number}: {error}"
                ) from error

            if not isinstance(language, str) or not language:
                raise ValueError(
                    f"Invalid language at {dataset}:{line_number}: {language!r}"
                )
            if label not in (0, 1):
                raise ValueError(f"Invalid label at {dataset}:{line_number}: {label!r}")
            counts[language][label] += 1

    return dict(counts)


def print_breakdown(counts: dict[str, dict[int, int]], output: TextIO) -> None:
    rows = [
        (language, by_label[0], by_label[1], by_label[0] + by_label[1])
        for language, by_label in sorted(counts.items())
    ]
    total_human = sum(row[1] for row in rows)
    total_ai = sum(row[2] for row in rows)
    rows.append(("TOTAL", total_human, total_ai, total_human + total_ai))

    language_width = max(len("Language"), *(len(row[0]) for row in rows))
    human_width = max(len("Human"), *(len(f"{row[1]:,}") for row in rows))
    ai_width = max(len("AI"), *(len(f"{row[2]:,}") for row in rows))
    total_width = max(len("Total"), *(len(f"{row[3]:,}") for row in rows))

    print(
        f"{'Language':<{language_width}}  {'Human':>{human_width}}  "
        f"{'AI':>{ai_width}}  {'Total':>{total_width}}",
        file=output,
    )
    print(
        f"{'-' * language_width}  {'-' * human_width}  "
        f"{'-' * ai_width}  {'-' * total_width}",
        file=output,
    )
    for language, human, ai, total in rows:
        print(
            f"{language:<{language_width}}  {human:>{human_width},}  "
            f"{ai:>{ai_width},}  {total:>{total_width},}",
            file=output,
        )


def main() -> None:
    args = parse_args()
    print_breakdown(count_chunks(args.dataset), output=sys.stdout)


if __name__ == "__main__":
    main()
