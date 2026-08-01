"""GitHub repository URL parsing and archive downloads."""

from __future__ import annotations

import re
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path, PurePosixPath
from typing import Any
from urllib.parse import quote, urlsplit


def github_repository(git_url: str) -> tuple[str, str] | None:
    """Return ``(owner, repository)`` for github.com repository URLs."""
    value = git_url.strip().rstrip("/")

    # Handle Git's scp-style syntax, which urlsplit does not parse as a URL.
    scp_match = re.fullmatch(
        r"(?:[^@/:]+@)?github\.com:([^/]+)/([^/]+)", value, flags=re.IGNORECASE
    )
    if scp_match:
        owner, repository = scp_match.groups()
    else:
        parsed = urlsplit(value)
        if parsed.hostname is None or parsed.hostname.casefold() != "github.com":
            return None
        parts = [part for part in parsed.path.split("/") if part]
        if len(parts) != 2:
            return None
        owner, repository = parts

    if repository.casefold().endswith(".git"):
        repository = repository[:-4]
    if not owner or not repository:
        return None
    return owner, repository


def archive_member_is_included(
    member: tarfile.TarInfo,
    include_paths: list[str],
    exclude_paths: list[str] | None = None,
) -> bool:
    parts = PurePosixPath(member.name).parts
    if len(parts) <= 1:
        return True
    relative = PurePosixPath(*parts[1:])

    for exclude_path in exclude_paths or []:
        excluded = PurePosixPath(exclude_path.strip("/"))
        if relative == excluded or relative.is_relative_to(excluded):
            return False

    if not include_paths:
        return True
    for include_path in include_paths:
        included = PurePosixPath(include_path.strip("/"))
        if relative == included or relative.is_relative_to(included):
            return True
        # Include directory entries leading to a selected path.
        if included.is_relative_to(relative):
            return True
    return False


def download_github_archive(
    source: dict[str, Any], cache_dir: Path, refresh: bool, owner: str, repository: str
) -> Path:
    """Download and extract a pinned repository tarball using ``gh api``."""
    destination = cache_dir / source["id"]
    revision_file = destination / ".vibesense-revision"
    if not refresh and revision_file.is_file():
        if revision_file.read_text(encoding="utf-8").strip() == source["revision"]:
            return destination

    destination.parent.mkdir(parents=True, exist_ok=True)
    endpoint = (
        f"repos/{quote(owner, safe='')}/{quote(repository, safe='')}/tarball/"
        f"{quote(source['revision'], safe='')}"
    )

    with tempfile.TemporaryDirectory(
        prefix=f".{source['id']}-", dir=destination.parent
    ) as temporary_directory:
        temporary_path = Path(temporary_directory)
        archive_path = temporary_path / "repository.tar.gz"
        extracted_path = temporary_path / "extracted"
        extracted_path.mkdir()

        with archive_path.open("wb") as archive_output:
            try:
                result = subprocess.run(
                    ["gh", "api", endpoint],
                    check=False,
                    stdout=archive_output,
                    stderr=subprocess.PIPE,
                )
            except OSError as error:
                raise RuntimeError(f"Could not run gh api: {error}") from error
        if result.returncode != 0:
            detail = result.stderr.decode(errors="replace").strip() or "gh api failed"
            raise RuntimeError(f"Could not download {source['url']!r}: {detail}")

        try:
            with tarfile.open(archive_path, mode="r:gz") as archive:
                members = archive.getmembers()
                roots = {
                    PurePosixPath(member.name).parts[0]
                    for member in members
                    if PurePosixPath(member.name).parts
                }
                if len(roots) != 1:
                    raise RuntimeError(
                        "GitHub archive does not have one top-level directory"
                    )
                selected_members = [
                    member
                    for member in members
                    if archive_member_is_included(
                        member,
                        source.get("include_paths", []),
                        source.get("exclude_paths", []),
                    )
                ]
                archive.extractall(
                    extracted_path, members=selected_members, filter="data"
                )
        except (tarfile.TarError, OSError) as error:
            raise RuntimeError(
                f"Could not extract archive for {source['id']!r}: {error}"
            ) from error

        extracted_repository = extracted_path / roots.pop()
        if not extracted_repository.is_dir():
            raise RuntimeError(f"GitHub archive for {source['id']!r} is empty")
        (extracted_repository / ".vibesense-revision").write_text(
            source["revision"] + "\n", encoding="utf-8"
        )

        if destination.is_symlink() or (
            destination.exists() and not destination.is_dir()
        ):
            destination.unlink()
        elif destination.exists():
            shutil.rmtree(destination)
        extracted_repository.replace(destination)

    return destination
