"""Shared helpers for talking to GitHub over HTTPS instead of the Git protocol."""

from __future__ import annotations

import json
import os
import re
import shutil
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any
from urllib.parse import urlsplit

API_ROOT = "https://api.github.com"
ARCHIVE_ROOT = "https://codeload.github.com"
USER_AGENT = "vibesense-dataset"
TIMEOUT_SECONDS = 60
DOWNLOAD_CHUNK_BYTES = 1 << 20

FULL_REVISION_PATTERN = re.compile(r"[0-9a-f]{40,64}")


class GitHubError(RuntimeError):
    """A GitHub request failed; callers fall back to the Git protocol."""


def github_repository(git_url: str) -> tuple[str, str] | None:
    """Return (owner, repository) when the URL points at github.com."""
    value = git_url.strip().rstrip("/")

    # urlsplit treats scp-style URLs (git@github.com:owner/repo.git) as paths.
    if "://" not in value and ":" in value:
        host, _, path = value.partition(":")
        host = host.rsplit("@", 1)[-1]
    else:
        split = urlsplit(value)
        if split.scheme not in {"http", "https", "ssh", "git"}:
            return None
        host = split.hostname or ""
        path = split.path

    if host.casefold() not in {"github.com", "www.github.com"}:
        return None

    parts = [part for part in path.strip("/").split("/") if part]
    if len(parts) != 2:
        return None

    owner, repository = parts
    if repository.casefold().endswith(".git"):
        repository = repository[:-4]
    if not owner or not repository:
        return None
    return owner, repository


def authorization_headers() -> dict[str, str]:
    headers = {"User-Agent": USER_AGENT}
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        # Authenticated requests raise the API rate limit from 60 to 5,000 per hour.
        headers["Authorization"] = f"Bearer {token}"
    return headers


def github_request(path: str) -> Any:
    """Perform an authenticated GitHub API request and decode the JSON response."""
    request = urllib.request.Request(
        f"{API_ROOT}{path}",
        headers={
            **authorization_headers(),
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=TIMEOUT_SECONDS) as response:
            payload = response.read()
    except (urllib.error.URLError, OSError) as error:
        raise GitHubError(f"GitHub API request failed for {path}: {error}") from error

    try:
        return json.loads(payload.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise GitHubError(f"GitHub API returned invalid JSON for {path}: {error}") from error


def is_pinned_revision(revision: str) -> bool:
    return bool(FULL_REVISION_PATTERN.fullmatch(revision or ""))


def archive_url(owner: str, repository: str, revision: str) -> str:
    return f"{ARCHIVE_ROOT}/{owner}/{repository}/tar.gz/{revision}"


def download_archive(url: str, destination: Path) -> Path:
    """Stream a source archive to destination, replacing it atomically."""
    destination.parent.mkdir(parents=True, exist_ok=True)
    partial = destination.with_name(destination.name + ".partial")
    request = urllib.request.Request(url, headers=authorization_headers())
    try:
        with urllib.request.urlopen(request, timeout=TIMEOUT_SECONDS) as response:
            with partial.open("wb") as output:
                shutil.copyfileobj(response, output, DOWNLOAD_CHUNK_BYTES)
    except (urllib.error.URLError, OSError) as error:
        partial.unlink(missing_ok=True)
        raise GitHubError(f"Could not download {url}: {error}") from error

    partial.replace(destination)
    return destination
