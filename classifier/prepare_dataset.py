from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import unicodedata
from collections import Counter, defaultdict
from collections.abc import Iterable
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Any

if __package__:
    from .github_archive import download_github_archive, github_repository
else:
    from github_archive import download_github_archive, github_repository

DATA_DIR = Path(__file__).resolve().parent / "data"

EXTENSION_LANGUAGES = {
    ".asm": "assembly",
    ".bash": "shell",
    ".c": "c",
    ".cc": "cpp",
    ".clj": "clojure",
    ".cljs": "clojure",
    ".cpp": "cpp",
    ".cs": "csharp",
    ".cxx": "cpp",
    ".dart": "dart",
    ".erl": "erlang",
    ".ex": "elixir",
    ".exs": "elixir",
    ".fish": "shell",
    ".fs": "fsharp",
    ".fsx": "fsharp",
    ".go": "go",
    ".h": "c",
    ".hh": "cpp",
    ".hpp": "cpp",
    ".hrl": "erlang",
    ".java": "java",
    ".js": "javascript",
    ".jsx": "javascript",
    ".kt": "kotlin",
    ".kts": "kotlin",
    ".lua": "lua",
    ".m": "objective-c",
    ".mm": "objective-cpp",
    ".php": "php",
    ".pl": "perl",
    ".ps1": "powershell",
    ".py": "python",
    ".r": "r",
    ".rb": "ruby",
    ".rs": "rust",
    ".s": "assembly",
    ".scala": "scala",
    ".sh": "shell",
    ".sol": "solidity",
    ".swift": "swift",
    ".ts": "typescript",
    ".tsx": "typescript",
    ".vb": "visual-basic",
    ".vbs": "visual-basic",
    ".zig": "zig",
    ".zsh": "shell",
}

SPECIAL_FILENAMES = {
    "dockerfile": "dockerfile",
    "makefile": "makefile",
    "rakefile": "ruby",
}

# Applied identically to every repository and sample directory.
EXCLUDED_PARTS = {
    ".cache",
    ".git",
    ".github",
    ".idea",
    ".venv",
    ".vscode",
    "__pycache__",
    "build",
    "coverage",
    "dist",
    "docs",
    "fixtures",
    "generated",
    "node_modules",
    "out",
    "target",
    "test",
    "tests",
    "third_party",
    "vendor",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DATA_DIR / "sources.json")
    parser.add_argument("--cache-dir", type=Path, default=DATA_DIR / ".cache" / "repos")
    parser.add_argument("--output-dir", type=Path, default=DATA_DIR / "processed")
    parser.add_argument("--max-file-bytes", type=int, default=256_000)
    parser.add_argument("--max-chars", type=int, default=12_000)
    parser.add_argument("--min-chars", type=int, default=40)
    parser.add_argument("--seed", default="vibesense-dataset-v1")
    parser.add_argument(
        "--workers",
        type=int,
        default=min(4, os.cpu_count() or 1),
        help="Files to normalize concurrently (default: up to 4)",
    )
    parser.add_argument(
        "--refresh", action="store_true", help="Fetch pinned revisions again"
    )
    parser.add_argument(
        "--no-balance",
        action="store_true",
        help="Do not balance AI/human snippets within each language in each split",
    )
    return parser.parse_args()


def run_git(*args: str, cwd: Path | None = None) -> None:
    subprocess.run(["git", *args], cwd=cwd, check=True)


def checkout_git_repository(
    source: dict[str, Any], cache_dir: Path, refresh: bool
) -> Path:
    destination = cache_dir / source["id"]
    git_dir = destination / ".git"

    if not git_dir.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        run_git(
            "clone",
            "--depth",
            "1",
            "--filter=blob:none",
            "--no-checkout",
            source["url"],
            str(destination),
        )
    elif refresh:
        run_git("fetch", "--depth", "1", "origin", source["revision"], cwd=destination)

    include_paths = source.get("include_paths", [])
    if include_paths:
        run_git("sparse-checkout", "init", "--cone", cwd=destination)
        run_git("sparse-checkout", "set", *include_paths, cwd=destination)

    try:
        run_git("checkout", "--detach", "--force", source["revision"], cwd=destination)
    except subprocess.CalledProcessError:
        run_git("fetch", "--depth", "1", "origin", source["revision"], cwd=destination)
        run_git("checkout", "--detach", "--force", source["revision"], cwd=destination)

    actual_revision = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=destination,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if actual_revision != source["revision"]:
        raise RuntimeError(
            f"{source['id']} resolved to {actual_revision}, expected {source['revision']}"
        )

    return destination


def checkout_repository(source: dict[str, Any], cache_dir: Path, refresh: bool) -> Path:
    github = github_repository(source["url"])
    if github is not None:
        return download_github_archive(source, cache_dir, refresh, *github)
    return checkout_git_repository(source, cache_dir, refresh)


def language_for(path: Path) -> str | None:
    special = SPECIAL_FILENAMES.get(path.name.casefold())
    if special:
        return special
    return EXTENSION_LANGUAGES.get(path.suffix.casefold())


def iter_code_files(root: Path) -> Iterable[Path]:
    paths: list[Path] = []
    directories = [root]

    while directories:
        directory = directories.pop()
        for path in directory.glob("*"):
            if path.is_symlink():
                continue
            if path.is_dir():
                if path.name.casefold() not in EXCLUDED_PARTS:
                    directories.append(path)
            elif path.is_file() and language_for(path) is not None:
                paths.append(path)

    yield from sorted(paths)


def normalize_code(raw: bytes) -> str | None:
    if b"\x00" in raw:
        return None
    try:
        text = raw.decode("utf-8-sig")
    except UnicodeDecodeError:
        return None

    text = unicodedata.normalize("NFC", text)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = [line.rstrip(" \t") for line in text.split("\n")]
    while lines and not lines[0]:
        lines.pop(0)
    while lines and not lines[-1]:
        lines.pop()
    return "\n".join(lines) + "\n" if lines else ""


# normalize_code converts CR/CRLF to LF. Keep a fallback for the less common
# separators recognized by str.splitlines so chunk_code's public behavior does
# not change for callers that pass text containing them.
_UNCOMMON_LINE_BREAK = re.compile(r"[\r\v\f\x1c-\x1e\x85\u2028\u2029]")


def _chunk_code_by_lines(text: str, max_chars: int) -> list[str]:
    chunks: list[str] = []
    current: list[str] = []
    current_size = 0

    for line in text.splitlines(keepends=True):
        if len(line) > max_chars:
            if current:
                chunks.append("".join(current))
                current = []
                current_size = 0
            chunks.extend(
                line[start : start + max_chars]
                for start in range(0, len(line), max_chars)
            )
            continue

        if current and current_size + len(line) > max_chars:
            chunks.append("".join(current))
            current = []
            current_size = 0
        current.append(line)
        current_size += len(line)

    if current:
        chunks.append("".join(current))
    return chunks


def chunk_code(text: str, max_chars: int) -> list[str]:
    if len(text) <= max_chars:
        return [text]
    if max_chars <= 0:
        raise ValueError("max_chars must be positive")
    if _UNCOMMON_LINE_BREAK.search(text) is not None:
        return _chunk_code_by_lines(text, max_chars)

    # Searching for one boundary per chunk runs in optimized C code. The old
    # implementation visited every source line in Python, which is costly for
    # repositories with tens of millions of short lines.
    chunks: list[str] = []
    start = 0
    text_size = len(text)
    while start < text_size:
        limit = min(start + max_chars, text_size)
        if limit == text_size:
            chunks.append(text[start:])
            break

        newline = text.rfind("\n", start, limit)
        if newline >= start:
            end = newline + 1
            chunks.append(text[start:end])
            start = end
            continue

        # The current line is longer than max_chars. As before, split that
        # line into fixed-width chunks without combining its tail with the
        # following line.
        line_end = text.find("\n", limit)
        line_end = text_size if line_end < 0 else line_end + 1
        while start < line_end:
            end = min(start + max_chars, line_end)
            chunks.append(text[start:end])
            start = end

    return chunks


def stable_digest(*parts: str) -> str:
    value = "\0".join(parts).encode("utf-8")
    return hashlib.sha256(value).hexdigest()


def records_for_source(
    source: dict[str, Any],
    root: Path,
    max_file_bytes: int,
    max_chars: int,
    min_chars: int,
    workers: int = 1,
) -> list[dict[str, Any]]:
    def records_for_file(path: Path) -> list[dict[str, Any]]:
        if path.stat().st_size > max_file_bytes:
            return []
        text = normalize_code(path.read_bytes())
        if text is None or len(text.strip()) < min_chars:
            return []

        language = language_for(path)
        relative_path = path.relative_to(root).as_posix()
        file_records: list[dict[str, Any]] = []
        for chunk_index, chunk in enumerate(chunk_code(text, max_chars)):
            if len(chunk.strip()) < min_chars:
                continue
            text_sha256 = hashlib.sha256(chunk.encode("utf-8")).hexdigest()
            record_id = stable_digest(
                source["id"], relative_path, str(chunk_index), text_sha256
            )
            file_records.append(
                {
                    "id": record_id,
                    "text": chunk,
                    "text_sha256": text_sha256,
                    "label": source["label"],
                    "label_name": source["label_name"],
                    "language": language,
                    "source_id": source["id"],
                    "source_kind": source["kind"],
                    "path": relative_path,
                    "chunk_index": chunk_index,
                    "repository_url": source.get("url"),
                    "revision": source.get("revision"),
                }
            )
        return file_records

    records: list[dict[str, Any]] = []
    paths = iter_code_files(root)
    if workers <= 1:
        record_groups = map(records_for_file, paths)
        for file_records in record_groups:
            records.extend(file_records)
        return records

    # Limit submitted work so a large repository does not create one Future
    # per file or retain many completed file results while preserving order.
    with ThreadPoolExecutor(max_workers=workers) as executor:
        record_groups = executor.map(
            records_for_file,
            paths,
            buffersize=workers * 2,
        )
        for file_records in record_groups:
            records.extend(file_records)
    return records


def deduplicate(records: list[dict[str, Any]]) -> tuple[list[dict[str, Any]], int, int]:
    unique: dict[str, dict[str, Any] | None] = {}
    duplicate_count = 0
    conflict_count = 0

    for record in records:
        digest = record["text_sha256"]
        previous = unique.get(digest)
        if digest not in unique:
            unique[digest] = record
        elif previous is not None and previous["label"] == record["label"]:
            duplicate_count += 1
        elif previous is not None:
            unique[digest] = None
            conflict_count += 1
        else:
            conflict_count += 1

    result = [record for record in unique.values() if record is not None]
    result.sort(
        key=lambda record: (record["source_id"], record["path"], record["chunk_index"])
    )
    return result, duplicate_count, conflict_count


def split_name(record: dict[str, Any], seed: str) -> str:
    # All chunks from one file receive the same split.
    digest = stable_digest(seed, record["source_id"], record["path"])
    bucket = int(digest[:8], 16) / 0xFFFFFFFF
    if bucket < 0.8:
        return "train"
    if bucket < 0.9:
        return "validation"
    return "test"


def balance_records(records: list[dict[str, Any]], seed: str) -> list[dict[str, Any]]:
    labels = {record["label"] for record in records}
    if labels != {0, 1}:
        raise RuntimeError(
            f"Cannot balance split; expected labels [0, 1], found {sorted(labels)}"
        )

    by_language: dict[str, dict[int, list[dict[str, Any]]]] = defaultdict(
        lambda: defaultdict(list)
    )
    for record in records:
        by_language[record["language"]][record["label"]].append(record)

    selected: list[dict[str, Any]] = []
    for language, by_label in sorted(by_language.items()):
        # A language represented by only one label cannot contribute to a
        # language-balanced dataset: its balanced sample size is zero.
        if set(by_label) != {0, 1}:
            continue

        target_size = min(len(by_label[0]), len(by_label[1]))
        for label in (0, 1):
            label_records = by_label[label]
            label_records.sort(
                key=lambda record: stable_digest(seed, language, record["id"])
            )
            selected.extend(label_records[:target_size])

    selected.sort(key=lambda record: stable_digest(seed, "output", record["id"]))
    return selected


def write_jsonl(path: Path, records: Iterable[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as output:
        for record in records:
            output.write(
                json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n"
            )


def validate_manifest(manifest: dict[str, Any]) -> None:
    source_ids: set[str] = set()
    for source in manifest.get("sources", []):
        source_id = source.get("id")
        if not source_id or source_id in source_ids:
            raise ValueError(f"Missing or duplicate source id: {source_id!r}")
        source_ids.add(source_id)
        if source.get("label") not in (0, 1):
            raise ValueError(f"Source {source_id} must use label 0 or 1")
        expected_name = "ai" if source["label"] == 1 else "human"
        if source.get("label_name") != expected_name:
            raise ValueError(f"Source {source_id} label_name must be {expected_name!r}")
        if source.get("kind") not in {"git", "github", "samples"}:
            raise ValueError(
                f"Unsupported source kind for {source_id}: {source.get('kind')!r}"
            )


def main() -> None:
    args = parse_args()
    manifest_path = args.manifest.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    validate_manifest(manifest)

    all_records: list[dict[str, Any]] = []
    source_counts: dict[str, int] = {}

    for source in manifest["sources"]:
        print(f"{source['id']}: start")
        if source["kind"] in {"git", "github"}:
            root = checkout_repository(source, args.cache_dir.resolve(), args.refresh)
        else:
            root = (manifest_path.parent / source["path"]).resolve()
            if not root.is_dir():
                raise FileNotFoundError(f"Sample directory does not exist: {root}")

        source_records = records_for_source(
            source,
            root,
            args.max_file_bytes,
            args.max_chars,
            args.min_chars,
            args.workers,
        )
        source_counts[source["id"]] = len(source_records)
        all_records.extend(source_records)
        print(f"{source['id']}: {len(source_records):,} normalized chunks")

    all_records, duplicate_count, conflict_count = deduplicate(all_records)
    for record in all_records:
        record["split"] = split_name(record, args.seed)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    write_jsonl(args.output_dir / "all.jsonl", all_records)

    split_counts: dict[str, dict[str, int]] = {}
    for name in ("train", "validation", "test"):
        split_records = [record for record in all_records if record["split"] == name]
        before_balance = Counter(record["label_name"] for record in split_records)
        if not args.no_balance:
            split_records = balance_records(split_records, f"{args.seed}-{name}")
        write_jsonl(args.output_dir / f"{name}.jsonl", split_records)
        split_counts[name] = dict(
            Counter(record["label_name"] for record in split_records)
        )
        print(
            f"{name}: {len(split_records):,} chunks {split_counts[name]} (before balance: {dict(before_balance)})"
        )

    try:
        summary_manifest = manifest_path.relative_to(Path.cwd()).as_posix()
    except ValueError:
        summary_manifest = manifest_path.as_posix()

    summary = {
        "manifest": summary_manifest,
        "seed": args.seed,
        "normalization": "NFC, UTF-8, LF line endings, trailing whitespace removed",
        "source_counts_before_deduplication": source_counts,
        "records_after_deduplication": len(all_records),
        "same_label_duplicates_removed": duplicate_count,
        "cross_label_conflicts_removed": conflict_count,
        "balanced": not args.no_balance,
        "split_counts": split_counts,
    }
    (args.output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
