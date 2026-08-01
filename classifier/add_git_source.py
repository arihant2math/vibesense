"""Add a pinned Git repository to the classifier source manifest.

When the manifest is prepared, snippets are balanced between AI and human labels
independently for every programming language.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit

if __package__:
    from .github_archive import download_github_archive, github_repository
else:
    from github_archive import download_github_archive, github_repository

DEFAULT_MANIFEST = Path(__file__).resolve().parent / "data" / "sources.train.json"
DEFAULT_CACHE_DIR = Path(__file__).resolve().parent / "data" / ".cache" / "repos"
CUTOFF_DATE = "2021-07-01T00:00:00Z"
# Git's --before comparison includes the named second, so query one second earlier.
CUTOFF_GIT_BEFORE = "2021-06-30T23:59:59Z"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("git_url", help="URL of the Git repository")
    parser.add_argument("label", type=int, choices=(0, 1), help="0 = human, 1 = AI")
    parser.add_argument(
        "--id",
        dest="source_id",
        help="Source ID (defaults to a slug derived from the repository name)",
    )
    revision_group = parser.add_mutually_exclusive_group()
    revision_group.add_argument(
        "--cutoff",
        action="store_true",
        help="Pin the last first-parent revision before July 1, 2021 UTC",
    )
    revision_group.add_argument(
        "--ref",
        help="Pin a branch, tag, ref, or commit instead of the repository HEAD",
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument(
        "--cache-dir",
        type=Path,
        default=DEFAULT_CACHE_DIR,
        help="Directory where GitHub repository archives are extracted",
    )
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


def git_environment() -> dict[str, str]:
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")
    return env


def run_git(
    arguments: list[str],
    *,
    cwd: Path | None = None,
    operation: str,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        ["git", *arguments],
        cwd=cwd,
        check=False,
        capture_output=True,
        text=True,
        env=git_environment(),
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or operation
        raise RuntimeError(f"{operation}: {detail}")
    return result


def validated_revision(value: str, description: str) -> str:
    revision = value.strip().splitlines()[0] if value.strip() else ""
    if not re.fullmatch(r"[0-9a-fA-F]{40,64}", revision):
        raise RuntimeError(f"Could not resolve {description}")
    return revision.lower()


def resolve_head(git_url: str) -> str:
    result = run_git(
        ["ls-remote", git_url, "HEAD"],
        operation=f"Could not read {git_url!r}",
    )
    line = next((line for line in result.stdout.splitlines() if line.strip()), "")
    revision = line.split(maxsplit=1)[0] if line else ""
    return validated_revision(revision, f"HEAD for {git_url!r}")


def temporary_bare_repository(
    git_url: str,
) -> tuple[tempfile.TemporaryDirectory[str], Path]:
    if not git_url or git_url.startswith("-"):
        raise ValueError("Git URL cannot be empty or start with '-'")

    temporary_directory = tempfile.TemporaryDirectory(prefix="vibesense-git-")
    repository = Path(temporary_directory.name) / "repository.git"
    try:
        run_git(
            ["init", "--bare", "--quiet", str(repository)],
            operation="Could not initialize a temporary Git repository",
        )
        run_git(
            ["remote", "add", "origin", git_url],
            cwd=repository,
            operation=f"Could not configure remote {git_url!r}",
        )
    except Exception:
        temporary_directory.cleanup()
        raise
    return temporary_directory, repository


def resolve_ref(git_url: str, ref: str) -> str:
    if not ref or ref.startswith("-"):
        raise ValueError("--ref cannot be empty or start with '-'")

    temporary_directory, repository = temporary_bare_repository(git_url)
    try:
        run_git(
            [
                "fetch",
                "--quiet",
                "--depth",
                "1",
                "--filter=blob:none",
                "--no-tags",
                "origin",
                ref,
            ],
            cwd=repository,
            operation=f"Could not fetch ref {ref!r} from {git_url!r}",
        )
        result = run_git(
            ["rev-parse", "--verify", "FETCH_HEAD^{commit}"],
            cwd=repository,
            operation=f"Ref {ref!r} does not resolve to a commit",
        )
        return validated_revision(result.stdout, f"ref {ref!r} for {git_url!r}")
    finally:
        temporary_directory.cleanup()


def resolve_cutoff(git_url: str) -> str:
    temporary_directory, repository = temporary_bare_repository(git_url)
    try:
        # A full commit history is needed to locate the cutoff, but blob contents
        # are filtered out on servers that support partial clone.
        run_git(
            [
                "fetch",
                "--quiet",
                "--filter=blob:none",
                "--no-tags",
                "origin",
                "HEAD",
            ],
            cwd=repository,
            operation=f"Could not fetch the default branch from {git_url!r}",
        )
        result = run_git(
            [
                "rev-list",
                "--first-parent",
                "-1",
                f"--before={CUTOFF_GIT_BEFORE}",
                "FETCH_HEAD",
            ],
            cwd=repository,
            operation=f"Could not inspect history for {git_url!r}",
        )
        if not result.stdout.strip():
            raise RuntimeError(
                f"No default-branch revision exists before {CUTOFF_DATE} for {git_url!r}"
            )
        return validated_revision(
            result.stdout,
            f"default-branch revision before {CUTOFF_DATE} for {git_url!r}",
        )
    finally:
        temporary_directory.cleanup()


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
        "kind": "github" if github_repository(git_url) else "git",
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
            raise ValueError(
                f"Source ID already exists: {source_id!r}; pass a unique --id"
            )
        if any(source.get("url") == args.git_url for source in manifest["sources"]):
            raise ValueError(f"Git URL is already in the manifest: {args.git_url!r}")

        if args.cutoff:
            revision = resolve_cutoff(args.git_url)
        elif args.ref is not None:
            revision = resolve_ref(args.git_url, args.ref)
        else:
            revision = resolve_head(args.git_url)
        source = add_source(manifest, args.git_url, args.label, source_id, revision)
        github = github_repository(args.git_url)
        if github is not None:
            download_github_archive(source, args.cache_dir.resolve(), False, *github)
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
