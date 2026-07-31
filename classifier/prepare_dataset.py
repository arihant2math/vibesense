from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tarfile
import time
import unicodedata
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Any, Iterable

try:
    from .github import (
        GitHubError,
        archive_url,
        download_archive,
        github_repository,
        is_pinned_revision,
    )
except ImportError:  # Support `python classifier/prepare_dataset.py`.
    from github import (
        GitHubError,
        archive_url,
        download_archive,
        github_repository,
        is_pinned_revision,
    )

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
    parser.add_argument("--refresh", action="store_true", help="Fetch pinned revisions again")
    parser.add_argument("--no-balance", action="store_true", help="Do not balance each split by label")
    parser.add_argument(
        "--max-chunks-per-source",
        type=int,
        default=2_000,
        help="Cap chunks contributed by one repository per language (0 disables)",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=8,
        help="Sources to fetch and read concurrently",
    )
    parser.add_argument(
        "--no-archives",
        action="store_true",
        help="Always clone over the Git protocol instead of downloading source archives",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    return args


def run_git(*args: str, cwd: Path | None = None) -> None:
    subprocess.run(["git", *args], cwd=cwd, check=True)


def checkout_repository(source: dict[str, Any], cache_dir: Path, refresh: bool) -> Path:
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


def archive_path(source: dict[str, Any], cache_dir: Path) -> Path:
    return cache_dir / "archives" / f"{source['id']}-{source['revision']}.tar.gz"


def fetch_archive(source: dict[str, Any], cache_dir: Path, refresh: bool) -> Path | None:
    """Download a pinned source archive over HTTPS; None when Git is required instead."""
    repository = github_repository(source.get("url") or "")
    if repository is None or not is_pinned_revision(source.get("revision") or ""):
        return None

    destination = archive_path(source, cache_dir)
    # Archives are named by commit, so a cached file always matches the pinned revision.
    if destination.is_file() and not refresh:
        return destination

    owner, name = repository
    try:
        return download_archive(archive_url(owner, name, source["revision"]), destination)
    except GitHubError as error:
        print(f"{source['id']}: falling back to git ({error})")
        return None


def matches_include_paths(relative: str, include_paths: list[str]) -> bool:
    """Mirror the cone-mode sparse checkout the Git path performs."""
    if not include_paths:
        return True

    directory = relative.rsplit("/", 1)[0] if "/" in relative else ""
    for include in include_paths:
        include = include.strip("/")
        if not include:
            continue
        if relative == include or relative.startswith(f"{include}/"):
            return True
        # Cone mode also keeps files sitting directly in a cone's parent directories.
        if directory == "" or include == directory or include.startswith(f"{directory}/"):
            return True
    return False


def is_selected(relative: str, include_paths: list[str]) -> bool:
    parts = relative.split("/")
    if any(part.casefold() in EXCLUDED_PARTS for part in parts[:-1]):
        return False
    if language_for(Path(relative)) is None:
        return False
    return matches_include_paths(relative, include_paths)


def iter_archive_files(tar_path: Path, include_paths: list[str], max_file_bytes: int):
    """Stream selected files out of a source archive without unpacking it to disk."""
    with tarfile.open(tar_path, "r:gz") as archive:
        for member in archive:
            # Regular files only; symlinks and directories are skipped as in the Git path.
            if not member.isfile() or member.size > max_file_bytes:
                continue
            # Archives are rooted at a single "<repository>-<revision>/" directory.
            _, separator, relative = member.name.partition("/")
            if not separator or not relative:
                continue
            if not is_selected(relative, include_paths):
                continue
            handle = archive.extractfile(member)
            if handle is None:
                continue
            yield relative, handle.read()


def language_for(path: Path) -> str | None:
    special = SPECIAL_FILENAMES.get(path.name.casefold())
    if special:
        return special
    return EXTENSION_LANGUAGES.get(path.suffix.casefold())


def iter_code_files(root: Path) -> Iterable[Path]:
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.is_symlink():
            continue
        relative = path.relative_to(root)
        if any(part.casefold() in EXCLUDED_PARTS for part in relative.parts[:-1]):
            continue
        if language_for(path) is not None:
            yield path


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


def chunk_code(text: str, max_chars: int) -> list[str]:
    if len(text) <= max_chars:
        return [text]

    chunks: list[str] = []
    current: list[str] = []
    current_size = 0

    for line in text.splitlines(keepends=True):
        if len(line) > max_chars:
            if current:
                chunks.append("".join(current))
                current = []
                current_size = 0
            chunks.extend(line[start : start + max_chars] for start in range(0, len(line), max_chars))
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


def stable_digest(*parts: str) -> str:
    value = "\0".join(parts).encode("utf-8")
    return hashlib.sha256(value).hexdigest()


def records_for_file(
    source: dict[str, Any],
    relative_path: str,
    raw: bytes,
    max_chars: int,
    min_chars: int,
) -> list[dict[str, Any]]:
    text = normalize_code(raw)
    if text is None or len(text.strip()) < min_chars:
        return []

    records: list[dict[str, Any]] = []
    for chunk_index, chunk in enumerate(chunk_code(text, max_chars)):
        if len(chunk.strip()) < min_chars:
            continue
        text_sha256 = hashlib.sha256(chunk.encode("utf-8")).hexdigest()
        record_id = stable_digest(source["id"], relative_path, str(chunk_index), text_sha256)
        records.append(
            {
                "id": record_id,
                "text": chunk,
                "text_sha256": text_sha256,
                "label": source["label"],
                "label_name": source["label_name"],
                "language": language_for(Path(relative_path)),
                "source_id": source["id"],
                "source_kind": source["kind"],
                "path": relative_path,
                "chunk_index": chunk_index,
                "repository_url": source.get("url"),
                "revision": source.get("revision"),
            }
        )
    return records


def records_for_source(
    source: dict[str, Any],
    root: Path,
    max_file_bytes: int,
    max_chars: int,
    min_chars: int,
) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []

    for path in iter_code_files(root):
        if path.stat().st_size > max_file_bytes:
            continue
        relative_path = path.relative_to(root).as_posix()
        records.extend(
            records_for_file(source, relative_path, path.read_bytes(), max_chars, min_chars)
        )

    return records


def records_for_archive(
    source: dict[str, Any],
    tar_path: Path,
    max_file_bytes: int,
    max_chars: int,
    min_chars: int,
) -> list[dict[str, Any]]:
    include_paths = source.get("include_paths", [])
    records: list[dict[str, Any]] = []
    for relative_path, raw in iter_archive_files(tar_path, include_paths, max_file_bytes):
        records.extend(records_for_file(source, relative_path, raw, max_chars, min_chars))
    records.sort(key=lambda record: (record["path"], record["chunk_index"]))
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
    result.sort(key=lambda record: (record["source_id"], record["path"], record["chunk_index"]))
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


def cap_by_source(
    records: list[dict[str, Any]], cap: int, seed: str
) -> tuple[list[dict[str, Any]], dict[str, int]]:
    """Limit how many chunks any one repository contributes to a language.

    Without this a single repository dominates its language — linux alone supplied 95k of
    the 117k human C chunks — so the model learns that repository's house style rather than
    anything about authorship.
    """
    if cap <= 0:
        return records, {}

    groups: dict[tuple[str, str | None], list[dict[str, Any]]] = defaultdict(list)
    for record in records:
        groups[(record["source_id"], record["language"])].append(record)

    kept: list[dict[str, Any]] = []
    trimmed: dict[str, int] = {}
    for (source_id, _), group in groups.items():
        if len(group) <= cap:
            kept.extend(group)
            continue
        group.sort(key=lambda record: stable_digest(seed, "cap", record["id"]))
        kept.extend(group[:cap])
        trimmed[source_id] = trimmed.get(source_id, 0) + len(group) - cap

    kept.sort(key=lambda record: (record["source_id"], record["path"], record["chunk_index"]))
    return kept, trimmed


def even_sample(
    records: list[dict[str, Any]], target: int, seed: str
) -> list[dict[str, Any]]:
    """Take target records, spreading the draw as evenly as possible across sources.

    Sources with fewer records than their share contribute everything they have and the
    shortfall is redistributed over the sources that still have depth.
    """
    pools: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in records:
        pools[record["source_id"]].append(record)
    for source_records in pools.values():
        source_records.sort(key=lambda record: stable_digest(seed, record["id"]))

    taken = {source_id: 0 for source_id in pools}
    selected: list[dict[str, Any]] = []

    while len(selected) < target:
        hungry = [
            source_id for source_id in sorted(pools) if taken[source_id] < len(pools[source_id])
        ]
        if not hungry:
            break
        share = max(1, (target - len(selected)) // len(hungry))
        for source_id in hungry:
            if len(selected) >= target:
                break
            available = len(pools[source_id]) - taken[source_id]
            count = min(share, available, target - len(selected))
            selected.extend(pools[source_id][taken[source_id] : taken[source_id] + count])
            taken[source_id] += count

    return selected


def balance_records(
    records: list[dict[str, Any]], seed: str
) -> tuple[list[dict[str, Any]], dict[str, int]]:
    """Balance labels within each language so language cannot leak the label.

    A language present under only one label is dropped entirely: keeping it would let a
    model score it correctly from syntax alone. Returns the balanced records and the
    number of chunks discarded per dropped language.
    """
    by_language: dict[str | None, dict[int, list[dict[str, Any]]]] = defaultdict(
        lambda: defaultdict(list)
    )
    for record in records:
        by_language[record["language"]][record["label"]].append(record)

    selected: list[dict[str, Any]] = []
    dropped: dict[str, int] = {}

    for language in sorted(by_language, key=lambda value: (value is None, value or "")):
        by_label = by_language[language]
        target_size = min(len(by_label.get(0, [])), len(by_label.get(1, [])))
        if target_size == 0:
            dropped[str(language)] = sum(len(group) for group in by_label.values())
            continue
        for _, label_records in sorted(by_label.items()):
            # Spread each label's quota across repositories instead of taking the largest.
            selected.extend(even_sample(label_records, target_size, seed))

    if not selected:
        raise RuntimeError(
            "Cannot balance split; no language has chunks under both labels"
        )

    selected.sort(key=lambda record: stable_digest(seed, "output", record["id"]))
    return selected, dropped


def write_jsonl(path: Path, records: Iterable[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as output:
        for record in records:
            output.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")


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
            raise ValueError(f"Unsupported source kind for {source_id}: {source.get('kind')!r}")


def main() -> None:
    args = parse_args()
    manifest_path = args.manifest.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    validate_manifest(manifest)

    cache_dir = args.cache_dir.resolve()

    def collect(source: dict[str, Any]) -> tuple[list[dict[str, Any]], str]:
        """Return this source's records plus a short description of how it was fetched."""
        if source["kind"] in {"git", "github"}:
            was_cached = archive_path(source, cache_dir).is_file() and not args.refresh
            tar_path = None if args.no_archives else fetch_archive(source, cache_dir, args.refresh)
            if tar_path is not None:
                records = records_for_archive(
                    source,
                    tar_path,
                    args.max_file_bytes,
                    args.max_chars,
                    args.min_chars,
                )
                verb = "cached" if was_cached else "downloaded"
                return records, f"{verb} {tar_path.stat().st_size / 1e6:.1f} MB"
            root = checkout_repository(source, cache_dir, args.refresh)
            fetch_note = "git clone"
        else:
            root = (manifest_path.parent / source["path"]).resolve()
            if not root.is_dir():
                raise FileNotFoundError(f"Sample directory does not exist: {root}")
            fetch_note = "local samples"

        records = records_for_source(
            source,
            root,
            args.max_file_bytes,
            args.max_chars,
            args.min_chars,
        )
        return records, fetch_note

    def timed_collect(source: dict[str, Any]) -> tuple[list[dict[str, Any]], str, float]:
        started = time.perf_counter()
        records, fetch_note = collect(source)
        return records, fetch_note, time.perf_counter() - started

    sources = manifest["sources"]
    started = time.perf_counter()
    workers = min(args.jobs, max(len(sources), 1))
    print(f"Reading {len(sources)} sources with {workers} worker(s)", flush=True)

    collected: list[list[dict[str, Any]]] = [[] for _ in sources]
    # Downloading and gzip-decoding both release the GIL, so threads overlap cleanly.
    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures = {pool.submit(timed_collect, source): index for index, source in enumerate(sources)}
        # Report each source the moment it lands rather than after the whole pool drains.
        for finished, future in enumerate(as_completed(futures), start=1):
            index = futures[future]
            source = sources[index]
            records, fetch_note, elapsed = future.result()
            collected[index] = records
            print(
                f"[{finished}/{len(sources)}] {source['id']}: "
                f"{len(records):,} chunks ({fetch_note}, {elapsed:.1f}s)",
                flush=True,
            )

    all_records: list[dict[str, Any]] = []
    source_counts: dict[str, int] = {}
    for source, source_records in zip(sources, collected):
        source_counts[source["id"]] = len(source_records)
        all_records.extend(source_records)
    print(
        f"Read {len(all_records):,} chunks from {len(sources)} sources "
        f"in {time.perf_counter() - started:.1f}s",
        flush=True,
    )

    all_records, duplicate_count, conflict_count = deduplicate(all_records)
    print(
        f"Deduplicated to {len(all_records):,} chunks "
        f"({duplicate_count:,} same-label duplicates, {conflict_count:,} cross-label conflicts removed)",
        flush=True,
    )
    for record in all_records:
        record["split"] = split_name(record, args.seed)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    # all.jsonl is the complete pool; caps and balancing shape only the split files.
    write_jsonl(args.output_dir / "all.jsonl", all_records)

    pool_records, trimmed = cap_by_source(all_records, args.max_chunks_per_source, args.seed)
    if trimmed:
        detail = ", ".join(
            f"{source_id} (-{count:,})"
            for source_id, count in sorted(trimmed.items(), key=lambda item: -item[1])[:6]
        )
        print(
            f"Capped at {args.max_chunks_per_source:,} chunks per repository per language: "
            f"{sum(trimmed.values()):,} dropped across {len(trimmed)} sources [{detail}]",
            flush=True,
        )

    split_counts: dict[str, dict[str, int]] = {}
    dropped_languages: dict[str, dict[str, int]] = {}
    for name in ("train", "validation", "test"):
        split_records = [record for record in pool_records if record["split"] == name]
        before_balance = Counter(record["label_name"] for record in split_records)
        if not args.no_balance:
            split_records, dropped = balance_records(split_records, f"{args.seed}-{name}")
            dropped_languages[name] = dropped
        write_jsonl(args.output_dir / f"{name}.jsonl", split_records)
        split_counts[name] = dict(Counter(record["label_name"] for record in split_records))
        print(f"{name}: {len(split_records):,} chunks {split_counts[name]} (before balance: {dict(before_balance)})")
        dropped = dropped_languages.get(name, {})
        if dropped:
            detail = ", ".join(
                f"{language} ({count:,})"
                for language, count in sorted(dropped.items(), key=lambda item: -item[1])
            )
            print(f"  dropped {sum(dropped.values()):,} chunks in single-label languages: {detail}")

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
        "balance_strategy": (
            "per-language label balance with per-source caps and even per-source draw; "
            "single-label languages dropped"
        ),
        "max_chunks_per_source_per_language": args.max_chunks_per_source,
        "chunks_trimmed_by_source_cap": trimmed,
        "dropped_single_label_languages": dropped_languages,
        "split_counts": split_counts,
    }
    (args.output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
