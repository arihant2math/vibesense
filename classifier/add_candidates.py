"""Append verified Claude-authored repositories to the source manifest.

Revisions are pinned in bulk through one GraphQL call per batch rather than a `git
ls-remote` per repository, and each entry records the commit-trailer evidence that earned
its label.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path
from typing import Any

DATA_DIR = Path(__file__).resolve().parent / "data"
DEFAULT_CANDIDATES = DATA_DIR / "claude_repo_candidates.json"
DEFAULT_MANIFEST = DATA_DIR / "sources.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidates", type=Path, default=DEFAULT_CANDIDATES)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--batch", type=int, default=20, help="Repositories per GraphQL call")
    parser.add_argument(
        "--min-trailer-fraction",
        type=float,
        default=0.0,
        help="Skip candidates below this fraction (already filtered during harvest)",
    )
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def resolve_revisions(full_names: list[str], batch_size: int) -> dict[str, str]:
    """Map each repository to its default-branch head commit."""
    revisions: dict[str, str] = {}
    for start in range(0, len(full_names), batch_size):
        batch = full_names[start : start + batch_size]
        parts = []
        for index, full_name in enumerate(batch):
            owner, _, name = full_name.partition("/")
            parts.append(
                f"  r{index}: repository(owner: {json.dumps(owner)}, name: {json.dumps(name)}) "
                "{ nameWithOwner defaultBranchRef { name target { oid } } }"
            )
        query = "query {\n" + "\n".join(parts) + "\n}"
        result = subprocess.run(
            ["gh", "api", "graphql", "-f", f"query={query}"],
            check=False, capture_output=True, text=True,
        )
        if result.returncode != 0:
            print(f"  batch failed: {result.stderr.strip()[:120]}")
            continue

        data = (json.loads(result.stdout).get("data") or {})
        for index, full_name in enumerate(batch):
            node = data.get(f"r{index}") or {}
            branch = node.get("defaultBranchRef") or {}
            oid = (branch.get("target") or {}).get("oid")
            if oid:
                revisions[full_name] = oid
        print(f"  pinned {len(revisions)}/{len(full_names)}", flush=True)
    return revisions


def source_id_for(full_name: str, taken: set[str]) -> str:
    owner, _, name = full_name.partition("/")
    base = re.sub(r"[^a-z0-9]+", "-", name.casefold()).strip("-") or "repo"
    if base not in taken:
        return base
    # Disambiguate with the owner when two repositories share a name.
    qualified = re.sub(r"[^a-z0-9]+", "-", f"{owner}-{name}".casefold()).strip("-")
    candidate, suffix = qualified, 2
    while candidate in taken:
        candidate = f"{qualified}-{suffix}"
        suffix += 1
    return candidate


def main() -> None:
    args = parse_args()
    payload = json.loads(args.candidates.read_text(encoding="utf-8"))
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    sources = manifest["sources"]

    existing_urls = {
        (source.get("url") or "").rstrip("/").removesuffix(".git").casefold()
        for source in sources
    }
    taken_ids = {source.get("id") for source in sources if source.get("id")}

    wanted = [
        entry
        for entry in payload["candidates"]
        if entry["trailer_fraction"] >= args.min_trailer_fraction
        and f"https://github.com/{entry['full_name']}".casefold() not in existing_urls
    ]
    print(f"{len(wanted)} candidates to add; resolving revisions", flush=True)

    revisions = resolve_revisions([entry["full_name"] for entry in wanted], args.batch)

    added = 0
    for entry in wanted:
        revision = revisions.get(entry["full_name"])
        if not revision:
            print(f"  skipped (no revision): {entry['full_name']}")
            continue
        source_id = source_id_for(entry["full_name"], taken_ids)
        taken_ids.add(source_id)
        sources.append(
            {
                "id": source_id,
                "kind": "git",
                "label": 1,
                "label_name": "ai",
                "url": f"https://github.com/{entry['full_name']}",
                "revision": revision,
                "provenance_note": (
                    "Claude Code commit trailer (noreply@anthropic.com) on "
                    f"{entry['trailer_fraction']:.0%} of {entry['commits_inspected']} commits "
                    "inspected; label inferred from that evidence, not independently verified."
                ),
            }
        )
        added += 1

    print(f"adding {added} sources ({len(sources)} total)")
    if args.dry_run:
        print("dry run; manifest unchanged")
        return

    args.manifest.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"wrote {args.manifest}")


if __name__ == "__main__":
    main()
