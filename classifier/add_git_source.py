"""Add a pinned Git repository to the classifier source manifest."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit

DEFAULT_MANIFEST = Path(__file__).resolve().parent / "data" / "sources.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("git_url", help="URL of the Git repository")
    parser.add_argument("label", type=int, choices=(0, 1), help="0 = human, 1 = AI")
    parser.add_argument(
        "--id",
        dest="source_id",
        help="Source ID (defaults to a slug derived from the repository name)",
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    return parser.parse_args()


def source_id_from_url(git_url: str) -> str:
    value = git_url.rstrip("/")
    path = urlsplit(value).path

    # urlsplit treats scp-style URLs (git@example.com:owner/repo.git) as paths.
    if ":" in path and "://" not in value:
        path = path.rsplit(":", 1)[-1]

    repository_name = path.rsplit("/", 1)[-1]
    if repository_name.casefold().endswith(".git"):
        repository_name = repository_name[:-4]

    source_id = re.sub(r"[^a-z0-9]+", "-", repository_name.casefold()).strip("-")
    if not source_id:
        raise ValueError(f"Could not derive a source ID from {git_url!r}; pass --id")
    return source_id


def resolve_head(git_url: str) -> str:
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")
    result = subprocess.run(
        ["git", "ls-remote", git_url, "HEAD"],
        check=False,
        capture_output=True,
        text=True,
        env=env,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or "git ls-remote failed"
        raise RuntimeError(f"Could not read {git_url!r}: {detail}")

    line = next((line for line in result.stdout.splitlines() if line.strip()), "")
    revision = line.split(maxsplit=1)[0] if line else ""
    if not re.fullmatch(r"[0-9a-fA-F]{40,64}", revision):
        raise RuntimeError(f"Could not resolve HEAD for {git_url!r}")
    return revision.lower()


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise ValueError(f"Manifest does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise ValueError(f"Manifest is not valid JSON: {path}: {error}") from error

    if not isinstance(manifest, dict) or not isinstance(manifest.get("sources"), list):
        raise ValueError(f"Manifest must contain a sources list: {path}")
    return manifest


def add_source(
    manifest: dict[str, Any], git_url: str, label: int, source_id: str, revision: str
) -> dict[str, Any]:
    sources = manifest["sources"]
    if any(source.get("id") == source_id for source in sources):
        raise ValueError(f"Source ID already exists: {source_id!r}; pass a unique --id")
    if any(source.get("url") == git_url for source in sources):
        raise ValueError(f"Git URL is already in the manifest: {git_url!r}")

    source = {
        "id": source_id,
        "kind": "git",
        "label": label,
        "label_name": "ai" if label == 1 else "human",
        "url": git_url,
        "revision": revision,
        "provenance_note": "Label supplied when this source was added; not independently verified.",
    }
    sources.append(source)
    return source


def main() -> None:
    args = parse_args()
    manifest_path = args.manifest.resolve()

    try:
        manifest = load_manifest(manifest_path)
        source_id = args.source_id or source_id_from_url(args.git_url)
        if not re.fullmatch(r"[a-z0-9]+(?:[._-][a-z0-9]+)*", source_id):
            raise ValueError(
                "Source ID must contain lowercase letters or digits separated by '.', '_', or '-'"
            )

        # Check cheap manifest conflicts before contacting the remote.
        if any(source.get("id") == source_id for source in manifest["sources"]):
            raise ValueError(f"Source ID already exists: {source_id!r}; pass a unique --id")
        if any(source.get("url") == args.git_url for source in manifest["sources"]):
            raise ValueError(f"Git URL is already in the manifest: {args.git_url!r}")

        revision = resolve_head(args.git_url)
        source = add_source(manifest, args.git_url, args.label, source_id, revision)
    except (RuntimeError, ValueError) as error:
        raise SystemExit(f"error: {error}") from error

    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(
        f"Added {source['id']} ({source['label_name']}) at {source['revision']} "
        f"to {manifest_path}"
    )


if __name__ == "__main__":
    main()
