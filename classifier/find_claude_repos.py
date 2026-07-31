"""Find repositories whose commits are authored by Claude Code, for AI-labeled sources.

Claude Code signs commits with a trailer:

    Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>

The model name varies, so `noreply@anthropic.com` is the stable marker. GitHub cannot
answer "which repositories have that trailer on every commit" directly, so this runs two
phases:

  discovery  commit search finds repositories with at least one Claude commit. Search is
             heavily secondary-rate-limited, so calls are paced and results checkpointed.
  verify     one batched GraphQL call per group of repositories reads the commit history
             and language mix, and keeps those whose trailer fraction clears a threshold.

Nothing is written to the source manifest; the output is a candidate list for review.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import time
from collections import defaultdict
from datetime import date, timedelta
from pathlib import Path
from typing import Any

DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_OUTPUT = DATA_DIR / "claude_repo_candidates.json"
PROCESSED_ALL = DATA_DIR / "processed" / "all.jsonl"

TRAILER_MARKER = "noreply@anthropic.com"
TRAILER_PATTERN = re.compile(r"co-authored-by:\s*claude[^\n]*noreply@anthropic\.com", re.IGNORECASE)

# GitHub's language names mapped onto the names prepare_dataset.py uses.
GITHUB_LANGUAGES = {
    "Assembly": "assembly",
    "C": "c",
    "C#": "csharp",
    "C++": "cpp",
    "Dockerfile": "dockerfile",
    "Elixir": "elixir",
    "F#": "fsharp",
    "Go": "go",
    "Java": "java",
    "JavaScript": "javascript",
    "Kotlin": "kotlin",
    "Makefile": "makefile",
    "Objective-C": "objective-c",
    "Objective-C++": "objective-cpp",
    "Perl": "perl",
    "PowerShell": "powershell",
    "Python": "python",
    "R": "r",
    "Ruby": "ruby",
    "Rust": "rust",
    "Shell": "shell",
    "Swift": "swift",
    "TypeScript": "typescript",
    "Zig": "zig",
}

# Search accepts a language qualifier on commits even though it is undocumented; it filters
# by the repository's primary language. Ordered by the quota table in the README.
DISCOVERY_LANGUAGES = [
    "c", "cpp", "python", "assembly", "java", "makefile", "zig",
    "go", "javascript", "shell", "objective-c", "perl", "rust",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--state",
        type=Path,
        default=DATA_DIR / ".cache" / "claude_harvest_state.json",
        help="Checkpoint file so an interrupted run resumes instead of restarting",
    )
    parser.add_argument("--repos-target", type=int, default=400, help="Distinct repos to discover")
    parser.add_argument("--pages-per-query", type=int, default=2, help="Search pages per slice")
    parser.add_argument("--search-delay", type=float, default=12.0, help="Seconds between searches")
    parser.add_argument("--graphql-batch", type=int, default=12, help="Repos per GraphQL call")
    parser.add_argument("--min-commits", type=int, default=5)
    parser.add_argument("--min-trailer-fraction", type=float, default=0.9)
    parser.add_argument("--max-history", type=int, default=300, help="Commits inspected per repo")
    parser.add_argument("--languages", nargs="*", default=DISCOVERY_LANGUAGES)
    parser.add_argument("--since", default="2025-01-01")
    parser.add_argument("--window-days", type=int, default=30)
    parser.add_argument("--cap", type=int, default=2000, help="Per-source cap used for quotas")
    parser.add_argument("--skip-discovery", action="store_true", help="Verify checkpointed repos only")
    return parser.parse_args()


def run_gh(arguments: list[str], *, attempts: int = 5) -> Any:
    """Call gh, backing off through GitHub's secondary rate limits."""
    delay = 60.0
    for attempt in range(attempts):
        result = subprocess.run(
            ["gh", *arguments], check=False, capture_output=True, text=True
        )
        if result.returncode == 0:
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError as error:
                raise RuntimeError(f"gh returned invalid JSON: {error}") from error

        stderr = result.stderr
        if "secondary rate limit" in stderr.lower() or "rate limit" in stderr.lower():
            if attempt == attempts - 1:
                break
            print(f"  rate limited; sleeping {delay:.0f}s", flush=True)
            time.sleep(delay)
            delay *= 2
            continue
        raise RuntimeError(f"gh failed: {stderr.strip()[:200]}")
    raise RuntimeError("gh failed: rate limited after retries")


def date_windows(since: str, window_days: int) -> list[tuple[str, str]]:
    start = date.fromisoformat(since)
    today = date.today()
    windows: list[tuple[str, str]] = []
    while start < today:
        end = min(start + timedelta(days=window_days - 1), today)
        windows.append((start.isoformat(), end.isoformat()))
        start = end + timedelta(days=1)
    # Newest windows first: recent repositories are likelier to be Claude-authored throughout.
    return list(reversed(windows))


def discover(args: argparse.Namespace, known: dict[str, str]) -> dict[str, str]:
    """Collect candidate repositories from commit search, newest windows first."""
    windows = date_windows(args.since, args.window_days)
    searches = 0
    truncated = 0

    for language in args.languages:
        for start, end in windows:
            if len(known) >= args.repos_target:
                print(
                    f"discovery: reached {args.repos_target} repos after {searches} searches "
                    f"({truncated} slices hit the 1,000-result cap and were sampled, not enumerated)",
                    flush=True,
                )
                return known
            for page in range(1, args.pages_per_query + 1):
                query = (
                    f'"{TRAILER_MARKER}" language:{language} '
                    f"committer-date:{start}..{end}"
                )
                path = f"/search/commits?q={query.replace(' ', '+').replace('\"', '%22')}"
                path += f"&per_page=100&page={page}"
                if searches:
                    time.sleep(args.search_delay)
                try:
                    payload = run_gh(["api", path])
                except RuntimeError as error:
                    print(f"  search failed ({language} {start}): {error}", flush=True)
                    break
                searches += 1

                items = payload.get("items", [])
                total = payload.get("total_count", 0)
                if total > 1000:
                    truncated += 1
                for item in items:
                    repository = item.get("repository") or {}
                    full_name = repository.get("full_name")
                    if full_name and full_name not in known:
                        known[full_name] = language
                if len(items) < 100:
                    break
            print(
                f"discovery: {language} {start}..{end} -> {len(known)} repos total",
                flush=True,
            )

    print(
        f"discovery: {len(known)} repos from {searches} searches "
        f"({truncated} slices exceeded the 1,000-result cap and were sampled)",
        flush=True,
    )
    return known


HISTORY_FRAGMENT = """
fragment repoFields on Repository {
  nameWithOwner
  stargazerCount
  isFork
  isArchived
  isEmpty
  diskUsage
  pushedAt
  licenseInfo { spdxId }
  primaryLanguage { name }
  languages(first: 8, orderBy: {field: SIZE, direction: DESC}) {
    edges { size node { name } }
  }
  defaultBranchRef {
    name
    target {
      ... on Commit {
        history(first: 100) {
          totalCount
          pageInfo { hasNextPage endCursor }
          nodes { message }
        }
      }
    }
  }
}
"""


def verify_batch(repositories: list[str]) -> dict[str, Any]:
    """Fetch history and language mix for a batch of repositories in one GraphQL call."""
    parts = []
    for index, full_name in enumerate(repositories):
        owner, _, name = full_name.partition("/")
        parts.append(
            f'  r{index}: repository(owner: {json.dumps(owner)}, name: {json.dumps(name)})'
            f" {{ ...repoFields }}"
        )
    query = HISTORY_FRAGMENT + "\nquery {\n" + "\n".join(parts) + "\n  rateLimit { remaining }\n}"
    payload = run_gh(["api", "graphql", "-f", f"query={query}"])
    return payload.get("data") or {}


def history_page(full_name: str, cursor: str) -> dict[str, Any]:
    owner, _, name = full_name.partition("/")
    query = """
    query($owner: String!, $name: String!, $cursor: String!) {
      repository(owner: $owner, name: $name) {
        defaultBranchRef { target { ... on Commit {
          history(first: 100, after: $cursor) {
            pageInfo { hasNextPage endCursor }
            nodes { message }
          }
        }}}
      }
    }"""
    payload = run_gh(
        [
            "api", "graphql",
            "-f", f"query={query}",
            "-F", f"owner={owner}",
            "-F", f"name={name}",
            "-F", f"cursor={cursor}",
        ]
    )
    data = (payload.get("data") or {}).get("repository") or {}
    branch = data.get("defaultBranchRef") or {}
    target = branch.get("target") or {}
    return target.get("history") or {}


def trailer_fraction(messages: list[str]) -> int:
    return sum(1 for message in messages if TRAILER_PATTERN.search(message or ""))


def evaluate(node: dict[str, Any], args: argparse.Namespace) -> dict[str, Any] | None:
    if not node or node.get("isEmpty") or node.get("isFork") or node.get("isArchived"):
        return None
    branch = node.get("defaultBranchRef") or {}
    target = branch.get("target") or {}
    history = target.get("history") or {}
    total = history.get("totalCount", 0)
    if total < args.min_commits:
        return None

    messages = [entry.get("message", "") for entry in history.get("nodes", [])]
    matched = trailer_fraction(messages)
    page_info = history.get("pageInfo") or {}

    # Only keep paging while the repository still looks fully Claude-authored.
    while (
        page_info.get("hasNextPage")
        and len(messages) < min(args.max_history, total)
        and matched / max(len(messages), 1) >= args.min_trailer_fraction
    ):
        page = history_page(node["nameWithOwner"], page_info["endCursor"])
        nodes = page.get("nodes", [])
        if not nodes:
            break
        messages.extend(entry.get("message", "") for entry in nodes)
        matched = trailer_fraction(messages)
        page_info = page.get("pageInfo") or {}

    fraction = matched / len(messages) if messages else 0.0
    if fraction < args.min_trailer_fraction:
        return None

    languages = {}
    for edge in (node.get("languages") or {}).get("edges", []):
        name = GITHUB_LANGUAGES.get((edge.get("node") or {}).get("name", ""))
        if name:
            languages[name] = edge.get("size", 0)

    primary = (node.get("primaryLanguage") or {}).get("name")
    return {
        "full_name": node["nameWithOwner"],
        "url": f"https://github.com/{node['nameWithOwner']}",
        "stars": node.get("stargazerCount", 0),
        "commits": total,
        "commits_inspected": len(messages),
        "trailer_fraction": round(fraction, 4),
        "primary_language": GITHUB_LANGUAGES.get(primary or "", primary),
        "languages": languages,
        "disk_usage_kb": node.get("diskUsage"),
        "license": (node.get("licenseInfo") or {}).get("spdxId"),
        "pushed_at": node.get("pushedAt"),
        "suspected_label": 1,
        "label_name": "ai",
    }


def load_quotas(cap: int) -> dict[str, int]:
    """Recompute the per-language AI shortfall from the prepared dataset."""
    if not PROCESSED_ALL.is_file():
        return {}
    counts: dict[str, dict[str, dict[str, int]]] = defaultdict(
        lambda: defaultdict(lambda: defaultdict(int))
    )
    with PROCESSED_ALL.open(encoding="utf-8") as handle:
        for line in handle:
            record = json.loads(line)
            counts[record["language"]][record["label_name"]][record["source_id"]] += 1

    quotas: dict[str, int] = {}
    for language, labels in counts.items():
        ai = sum(min(value, cap) for value in labels.get("ai", {}).values())
        human = sum(min(value, cap) for value in labels.get("human", {}).values())
        if human > ai:
            quotas[language] = human - ai
    return quotas


def main() -> None:
    args = parse_args()
    args.state.parent.mkdir(parents=True, exist_ok=True)

    state: dict[str, Any] = {"discovered": {}, "verified": [], "rejected": []}
    if args.state.is_file():
        state.update(json.loads(args.state.read_text(encoding="utf-8")))
        print(
            f"resuming: {len(state['discovered'])} discovered, "
            f"{len(state['verified'])} verified, {len(state['rejected'])} rejected",
            flush=True,
        )

    if not args.skip_discovery:
        state["discovered"] = discover(args, dict(state["discovered"]))
        args.state.write_text(json.dumps(state, indent=2), encoding="utf-8")

    seen = {entry["full_name"] for entry in state["verified"]} | set(state["rejected"])
    pending = [name for name in state["discovered"] if name not in seen]
    print(f"verify: {len(pending)} repositories to inspect", flush=True)

    for start in range(0, len(pending), args.graphql_batch):
        batch = pending[start : start + args.graphql_batch]
        try:
            data = verify_batch(batch)
        except RuntimeError as error:
            print(f"  batch failed: {error}", flush=True)
            continue

        for index, full_name in enumerate(batch):
            candidate = evaluate(data.get(f"r{index}") or {}, args)
            if candidate:
                state["verified"].append(candidate)
                print(
                    f"  KEEP {full_name} "
                    f"({candidate['trailer_fraction']:.0%} of {candidate['commits_inspected']} commits, "
                    f"{candidate['primary_language']}, {candidate['stars']}*)",
                    flush=True,
                )
            else:
                state["rejected"].append(full_name)
        args.state.write_text(json.dumps(state, indent=2), encoding="utf-8")
        print(
            f"verify: {min(start + args.graphql_batch, len(pending))}/{len(pending)} "
            f"-> {len(state['verified'])} candidates",
            flush=True,
        )

    quotas = load_quotas(args.cap)
    candidates = sorted(
        state["verified"], key=lambda entry: (-entry["trailer_fraction"], -entry["stars"])
    )
    by_language: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for candidate in candidates:
        by_language[candidate["primary_language"] or "unknown"].append(candidate)

    args.output.write_text(
        json.dumps(
            {
                "marker": TRAILER_MARKER,
                "min_trailer_fraction": args.min_trailer_fraction,
                "min_commits": args.min_commits,
                "quotas_at_cap": quotas,
                "candidates": candidates,
            },
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )

    print(f"\n{len(candidates)} candidates written to {args.output}\n")
    print(f"{'language':14} {'repos':>6} {'chunks est':>11} {'quota':>8}")
    for language, entries in sorted(by_language.items(), key=lambda item: -len(item[1])):
        # Rough: prepared repositories average around one chunk per 7.4 KB of code.
        estimate = sum(int((entry["disk_usage_kb"] or 0) / 7.4) for entry in entries)
        print(
            f"{language:14} {len(entries):6,} {estimate:11,} {quotas.get(language, 0):8,}"
        )


if __name__ == "__main__":
    main()
