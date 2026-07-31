"""Generate AI-labeled code samples by prompting a model through the `pi` CLI.

Sample counts are allocated across languages by how much human data each one currently
strands: per-language balancing keeps only min(ai, human) chunks, so a language with far
more human than AI chunks wastes the difference. Weights are damped with a square root so
C does not consume the entire run, and every target language gets a floor.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import re
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Any

DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_OUTPUT_DIR = DATA_DIR / "raw" / "ai" / "samples"
DEFAULT_MODEL = "openai-codex/gpt-5.6-luna"

# Chunks of human data each language currently strands for want of AI data, measured from
# data/processed/all.jsonl. Refresh with the language table in the README when sources change.
LANGUAGE_DEMAND = {
    "c": 114_209,
    "cpp": 24_628,
    "java": 23_038,
    "go": 5_733,
    "javascript": 4_336,
    "python": 4_142,
    "assembly": 3_614,
    "makefile": 2_712,
    "zig": 2_367,
    "shell": 957,
    "objective-cpp": 308,
    "objective-c": 218,
    "perl": 187,
    "dockerfile": 91,
    "csharp": 22,
    "kotlin": 15,
}

# Must agree with EXTENSION_LANGUAGES in prepare_dataset.py or the samples are ignored.
LANGUAGE_EXTENSIONS = {
    "assembly": ".s",
    "c": ".c",
    "cpp": ".cpp",
    "csharp": ".cs",
    "dockerfile": "",  # Written as a bare "Dockerfile"; see sample_filename.
    "go": ".go",
    "java": ".java",
    "javascript": ".js",
    "kotlin": ".kt",
    "makefile": "",  # Written as a bare "Makefile".
    "objective-c": ".m",
    "objective-cpp": ".mm",
    "perl": ".pl",
    "python": ".py",
    "shell": ".sh",
    "swift": ".swift",
    "typescript": ".ts",
    "zig": ".zig",
}

# Language names as they should appear in the prompt.
LANGUAGE_PROMPT_NAMES = {
    "c": "C",
    "go": "Go",
    "java": "Java",
    "javascript": "JavaScript",
    "kotlin": "Kotlin",
    "perl": "Perl",
    "python": "Python",
    "swift": "Swift",
    "typescript": "TypeScript",
    "zig": "Zig",
    "cpp": "C++",
    "csharp": "C#",
    "objective-c": "Objective-C",
    "objective-cpp": "Objective-C++",
    "makefile": "Make (a Makefile)",
    "dockerfile": "Docker (a Dockerfile)",
    "assembly": "x86-64 assembly (GNU as syntax)",
    "shell": "POSIX shell",
}

TASKS = [
    "implements a fixed-capacity circular buffer",
    "implements an LRU cache with a configurable size limit",
    "implements a binary search tree with insert, delete, and in-order traversal",
    "implements a min-heap priority queue",
    "implements a hash table with open addressing and linear probing",
    "implements a union-find structure with path compression",
    "implements a trie for prefix lookups",
    "implements a skip list with randomized levels",
    "implements a bloom filter with configurable false-positive rate",
    "implements a thread-safe bounded blocking queue",
    "parses a CSV file and reports per-column summary statistics",
    "parses command-line arguments into a typed configuration struct",
    "parses an INI configuration file into nested key-value pairs",
    "tokenizes and evaluates infix arithmetic expressions",
    "implements a recursive-descent parser for a small JSON subset",
    "implements a regular expression matcher supporting '.' and '*'",
    "implements a template renderer that substitutes {{name}} placeholders",
    "implements a Markdown-to-HTML converter for headings, lists, and emphasis",
    "implements run-length encoding and decoding",
    "implements Huffman coding for byte streams",
    "implements base64 encoding and decoding without library helpers",
    "computes CRC32 checksums over a byte buffer",
    "implements a SHA-256 style block hashing loop over fixed-size input",
    "implements a simple XOR-based stream cipher with a rotating key",
    "implements Dijkstra's shortest path over an adjacency list",
    "implements topological sort with cycle detection",
    "implements A* pathfinding on a 2D grid with obstacles",
    "implements breadth-first flood fill on a bitmap",
    "implements merge sort and quicksort with a shared comparison interface",
    "implements binary search over a rotated sorted array",
    "implements the Levenshtein edit distance with a rolling row",
    "implements longest common subsequence with backtracking",
    "implements a sliding-window rate limiter",
    "implements a token bucket throttle with refill over time",
    "implements an exponential backoff retry helper with jitter",
    "implements a fixed-size thread pool that executes queued jobs",
    "implements a producer-consumer pipeline with bounded handoff",
    "implements a worker that batches items until a size or time threshold",
    "implements a finite state machine for a vending machine",
    "implements a traffic light controller as an explicit state machine",
    "implements a simple event emitter with subscribe and unsubscribe",
    "implements an observer pattern for property change notifications",
    "implements a dependency injection container with singleton scopes",
    "implements a plugin registry that resolves handlers by name",
    "implements a circuit breaker that opens after repeated failures",
    "implements an in-memory key-value store with TTL expiry",
    "implements a write-ahead log with append and replay",
    "implements a paginated result cursor over an in-memory collection",
    "implements a directory walker that reports file sizes by extension",
    "implements a line-oriented log file tailer",
    "implements atomic file writes via a temporary file and rename",
    "implements a file deduplicator that groups files by content hash",
    "implements a fixed-point decimal type with add and multiply",
    "implements a 3D vector type with dot and cross products",
    "implements a 4x4 matrix multiply and transpose",
    "implements a pseudo-random number generator with a seedable state",
    "implements a moving average and standard deviation accumulator",
    "implements a histogram with configurable bucket boundaries",
    "implements a reservoir sampler over a stream of unknown length",
    "implements a date parser and formatter for ISO 8601 timestamps",
    "implements a duration parser that accepts strings like '1h30m'",
    "implements a semantic version type with comparison operators",
    "implements a URL parser that splits scheme, host, path, and query",
    "implements an HTTP query string encoder and decoder",
    "implements a simple TCP echo server",
    "implements a UDP heartbeat sender and receiver pair",
    "implements a Conway's Game of Life simulation on a toroidal grid",
    "implements a maze generator using randomized depth-first search",
    "implements a tic-tac-toe game with a minimax opponent",
    "implements a text-based inventory system with add, remove, and query",
    "implements a bank account ledger with deposits, withdrawals, and balance history",
    "implements a shopping cart with quantity updates and totals",
    "implements a task scheduler that runs callbacks at fixed intervals",
    "implements a stack-based virtual machine for a tiny instruction set",
    "implements an assembler for a toy instruction set",
    "implements a memory allocator over a fixed byte arena",
    "implements a reference-counted smart pointer",
    "implements a ring-buffer based logger with severity levels",
    "implements a matrix determinant via LU decomposition",
    "implements a spell checker using edit distance over a word list",
]

# Build files and assembly do not fit the general implementation tasks above.
TASK_OVERRIDES = {
    "makefile": [
        "builds a C project from a src directory with debug and release targets",
        "builds and links a static library, then a binary that consumes it",
        "compiles every .c file into an obj directory with automatic dependency tracking",
        "provides build, test, clean, and install targets for a C++ project",
        "cross-compiles a project for several target architectures",
        "runs a linter, formatter, and test suite as separate phony targets",
        "builds a project with configurable optimization and sanitizer flags",
        "generates documentation and packages the result into a tarball",
        "builds a kernel module against the running kernel headers",
        "vendors dependencies and builds a Go binary with version stamping",
        "runs a two-stage build that generates code before compiling it",
        "builds a shared library with a versioned soname and installs headers",
    ],
    "dockerfile": [
        "builds a multi-stage image for a compiled Go service on a distroless base",
        "packages a Python application with pinned dependencies and a non-root user",
        "builds a C++ project with a separate builder stage and a slim runtime stage",
        "creates a reproducible Node.js image with a cached dependency layer",
        "packages a Java application with a JRE-only runtime stage",
        "builds an image with a health check and configurable entrypoint arguments",
        "creates a development image with build tools and a mounted workspace",
        "packages a static binary into a scratch image with CA certificates",
        "builds a Rust binary with cached cargo dependencies and a minimal runtime",
        "creates an image that runs database migrations before starting the service",
    ],
    "assembly": [
        "computes the length of a null-terminated string",
        "copies a memory region forward with overlap handling",
        "sums an array of 64-bit integers",
        "finds the maximum value in an integer array",
        "reverses a byte buffer in place",
        "converts an integer to a decimal ASCII string",
        "parses a decimal ASCII string into an integer",
        "compares two memory regions byte by byte",
        "counts set bits across a buffer",
        "writes a string to standard output via a syscall",
        "computes a factorial iteratively",
        "swaps two integers through pointers",
        "fills a memory region with a repeating byte pattern",
        "searches a byte buffer for the first occurrence of a value",
    ],
}

MODIFIER_OVERRIDES = {
    "makefile": [
        "",
        ", with clear variable definitions for the compiler and flags",
        ", using pattern rules rather than repeated recipes",
        ", with phony targets declared explicitly",
        ", portable across GNU Make on Linux and macOS",
        ", with comments explaining each target",
    ],
    "dockerfile": [
        "",
        ", keeping the final image as small as possible",
        ", with layer ordering chosen for cache efficiency",
        ", running as a non-root user",
        ", with comments explaining each stage",
        ", pinning base image versions explicitly",
    ],
}

MODIFIERS = [
    "",
    ", with thorough input validation and error handling",
    ", using only the standard library",
    ", optimized for low memory usage",
    ", with documentation comments on every public function",
    ", written to be easy to unit test",
    ", handling edge cases like empty and single-element input",
    ", with a small demonstration in a main entry point",
    ", favoring clarity over cleverness",
    ", with defensive checks against integer overflow",
]

PROMPT_TEMPLATE = (
    "generate code in {language} that {task}{modifier}. "
    "only respond with the code and nothing else. "
    "do not include markdown code fences, explanations, or commentary."
)

FENCE_PATTERN = re.compile(r"^\s*```", re.MULTILINE)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=200, help="Samples to generate")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--thinking", default="low")
    parser.add_argument("--jobs", type=int, default=8, help="Concurrent model calls")
    parser.add_argument("--timeout", type=int, default=180, help="Seconds per call")
    parser.add_argument("--seed", default="vibesense-samples-v1")
    parser.add_argument("--min-chars", type=int, default=40)
    parser.add_argument(
        "--languages",
        nargs="*",
        help="Restrict generation to these languages (default: the demand table)",
    )
    parser.add_argument(
        "--retries", type=int, default=2, help="Retries per prompt on failure or empty output"
    )
    args = parser.parse_args()
    if args.count < 1:
        parser.error("--count must be at least 1")
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    return args


def allocate(count: int, languages: list[str]) -> dict[str, int]:
    """Split count across languages by damped demand, giving every language a floor."""
    weights = {name: math.sqrt(LANGUAGE_DEMAND.get(name, 1)) for name in languages}
    total_weight = sum(weights.values())

    floor = 1 if count < len(languages) * 3 else 3
    allocation = {name: min(floor, count) for name in languages}
    remaining = count - sum(allocation.values())
    if remaining <= 0:
        return {name: value for name, value in allocation.items() if value}

    for name in languages:
        allocation[name] += int(remaining * weights[name] / total_weight)

    # Hand out rounding remainder to the highest-demand languages first.
    shortfall = count - sum(allocation.values())
    for name in sorted(languages, key=lambda value: -weights[value])[: max(shortfall, 0)]:
        allocation[name] += 1
    return allocation


def sample_filename(language: str, index: int, slug: str) -> str:
    extension = LANGUAGE_EXTENSIONS[language]
    if language == "makefile":
        return f"{index:04d}_{slug}/Makefile"
    if language == "dockerfile":
        return f"{index:04d}_{slug}/Dockerfile"
    return f"{index:04d}_{slug}{extension}"


def slugify(task: str) -> str:
    words = re.sub(r"[^a-z0-9]+", "_", task.casefold()).strip("_").split("_")
    return "_".join(words[1:6]) or "sample"


def build_jobs(count: int, languages: list[str], seed: str) -> list[dict[str, Any]]:
    allocation = allocate(count, languages)
    rng = random.Random(seed)
    jobs: list[dict[str, Any]] = []

    for language in languages:
        wanted = allocation.get(language, 0)
        if not wanted:
            continue
        # Sample distinct (task, modifier) pairs so one language never repeats a prompt.
        tasks = TASK_OVERRIDES.get(language, TASKS)
        modifiers = MODIFIER_OVERRIDES.get(language, MODIFIERS)
        pairs = [(task, modifier) for task in tasks for modifier in modifiers]
        rng.shuffle(pairs)
        for index, (task, modifier) in enumerate(pairs[:wanted], start=1):
            prompt_language = LANGUAGE_PROMPT_NAMES.get(language, language)
            jobs.append(
                {
                    "language": language,
                    "task": task,
                    "modifier": modifier,
                    "relative_path": f"{language}/{sample_filename(language, index, slugify(task))}",
                    "prompt": PROMPT_TEMPLATE.format(
                        language=prompt_language, task=task, modifier=modifier
                    ),
                }
            )

    rng.shuffle(jobs)
    return jobs


def call_model(prompt: str, model: str, thinking: str, timeout: int) -> str:
    result = subprocess.run(
        [
            "pi",
            "--model", model,
            "--thinking", thinking,
            "-nt", "-ne", "-ns", "-nc", "-np", "-a",
            "-p", prompt,
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or "no output"
        raise RuntimeError(f"pi exited {result.returncode}: {detail[:200]}")
    return result.stdout


def generate(job: dict[str, Any], args: argparse.Namespace) -> dict[str, Any]:
    destination = args.output_dir / job["relative_path"]
    if destination.is_file():
        return {**job, "status": "skipped", "chars": destination.stat().st_size}

    last_error = "unknown"
    for attempt in range(args.retries + 1):
        try:
            code = call_model(job["prompt"], args.model, args.thinking, args.timeout)
        except (RuntimeError, subprocess.TimeoutExpired) as error:
            last_error = str(error)
            continue
        if len(code.strip()) < args.min_chars:
            last_error = f"output shorter than {args.min_chars} characters"
            continue

        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(code, encoding="utf-8")
        return {
            **job,
            "status": "generated",
            "chars": len(code),
            "attempts": attempt + 1,
            # Reported, not stripped: the prompt forbids fences, so their presence means
            # the model ignored the instruction and the sample needs a look.
            "has_fence": bool(FENCE_PATTERN.search(code)),
        }

    return {**job, "status": "failed", "error": last_error}


def main() -> None:
    args = parse_args()
    languages = args.languages or list(LANGUAGE_DEMAND)
    unknown = [name for name in languages if name not in LANGUAGE_EXTENSIONS]
    if unknown:
        raise SystemExit(f"error: unsupported language(s): {', '.join(unknown)}")

    jobs = build_jobs(args.count, languages, args.seed)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    started = time.perf_counter()
    print(f"Generating {len(jobs)} samples with {args.model} ({args.jobs} workers)", flush=True)

    results: list[dict[str, Any]] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(generate, job, args): job for job in jobs}
        for finished, future in enumerate(as_completed(futures), start=1):
            result = future.result()
            results.append(result)
            marker = {"generated": "ok", "skipped": "cached", "failed": "FAILED"}[result["status"]]
            detail = (
                f"{result.get('chars', 0):,} chars"
                if result["status"] != "failed"
                else result.get("error", "")[:70]
            )
            fence = " [contains fence]" if result.get("has_fence") else ""
            print(
                f"[{finished}/{len(jobs)}] {marker:7} {result['relative_path']} ({detail}){fence}",
                flush=True,
            )

    counts: dict[str, int] = {}
    for result in results:
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    fenced = [result for result in results if result.get("has_fence")]

    metadata = {
        "model": args.model,
        "thinking": args.thinking,
        "seed": args.seed,
        "prompt_template": PROMPT_TEMPLATE,
        "counts": counts,
        "samples": sorted(
            (
                {
                    "path": result["relative_path"],
                    "language": result["language"],
                    "task": result["task"],
                    "modifier": result["modifier"],
                    "prompt": result["prompt"],
                    "status": result["status"],
                }
                for result in results
            ),
            key=lambda entry: entry["path"],
        ),
    }
    (args.output_dir / "generation_metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    print(
        f"\n{counts.get('generated', 0)} generated, {counts.get('skipped', 0)} already present, "
        f"{counts.get('failed', 0)} failed in {time.perf_counter() - started:.1f}s"
    )
    if fenced:
        print(f"{len(fenced)} sample(s) contain markdown fences despite the prompt:")
        for result in fenced[:10]:
            print(f"  {result['relative_path']}")


if __name__ == "__main__":
    main()
