import json
import os
import re
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from os import PathLike
from pathlib import Path, PurePosixPath

import requests
from requests.utils import quote, urlparse


class FileType(Enum):
    FILE = "file"
    DIRECTORY = "dir"


@dataclass
class DirEntry:
    name: str
    entry_type: FileType
    size: int | None


class Accessor(ABC):
    @abstractmethod
    def list_dir(self, relative_path: str | PathLike[str]) -> list[DirEntry]:
        """List a directory"""

    @abstractmethod
    def read_file(
        self, relative_path: str | PathLike[str], offset=None, limit=None
    ) -> str:
        """Read file from start to end (either/both could be `None`)"""


class DirectoryAccessor(Accessor):
    """Access files below a directory on the local filesystem."""

    def __init__(self, directory: str | PathLike[str]) -> None:
        self.directory = Path(directory).expanduser().resolve(strict=True)
        if not self.directory.is_dir():
            raise NotADirectoryError(self.directory)

    def _resolve(self, relative_path: str | PathLike[str]) -> Path:
        relative = Path(relative_path)
        if relative.is_absolute():
            raise ValueError(f"Path must be relative: {relative_path!r}")

        path = (self.directory / relative).resolve(strict=False)
        try:
            path.relative_to(self.directory)
        except ValueError as error:
            raise ValueError(
                f"Path is outside the accessed directory: {relative_path!r}"
            ) from error
        return path

    def list_dir(self, relative_path: str | PathLike[str]) -> list[DirEntry]:
        directory = self._resolve(relative_path)
        if not directory.is_dir():
            if not directory.exists():
                raise FileNotFoundError(directory)
            raise NotADirectoryError(directory)

        result: list[DirEntry] = []
        for path in directory.iterdir():
            # Do not expose symlinks whose targets escape the configured root.
            try:
                resolved = path.resolve(strict=True)
                resolved.relative_to(self.directory)
            except FileNotFoundError, RuntimeError, ValueError:
                continue

            if resolved.is_dir():
                result.append(DirEntry(path.name, FileType.DIRECTORY, None))
            elif resolved.is_file():
                result.append(
                    DirEntry(path.name, FileType.FILE, resolved.stat().st_size)
                )
            # TODO: handle links and sockets etc

        return sorted(result, key=lambda entry: entry.name)

    def read_file(
        self,
        relative_path: str | PathLike[str],
        offset: int | None = None,
        limit: int | None = None,
    ) -> str:
        if offset is not None and (not isinstance(offset, int) or offset < 0):
            raise ValueError("offset must be a non-negative integer or None")
        if limit is not None and (not isinstance(limit, int) or limit < 0):
            raise ValueError("limit must be a non-negative integer or None")

        path = self._resolve(relative_path)
        if not path.is_file():
            if not path.exists():
                raise FileNotFoundError(path)
            if path.is_dir():
                raise IsADirectoryError(path)
            raise OSError(f"Not a regular file: {path}")

        with path.open(encoding="utf-8") as file:
            if offset:
                file.read(offset)
            return file.read() if limit is None else file.read(limit)


class GitHubAccessor(Accessor):
    """Access a GitHub repository without checking it out locally.

    ``source`` may be an ``owner/repository`` pair or a GitHub HTTPS, SSH, or
    scp-style Git URL. Alternatively, pass the owner as ``source`` and the
    repository name as ``repository``. The default branch is used unless
    ``ref`` is supplied. ``GITHUB_TOKEN`` or ``GH_TOKEN`` is used when present.
    """

    def __init__(
        self,
        source: str,
        repository: str | None = None,
        ref: str | None = None,
        *,
        token: str | None = None,
        api_url: str = "https://api.github.com",
        timeout: float = 30,
    ) -> None:
        if repository is None:
            owner, repository = self._parse_repository(source)
        else:
            owner = source.strip()
            repository = repository.strip()
            if repository.casefold().endswith(".git"):
                repository = repository[:-4]
            if not owner or not repository or "/" in owner or "/" in repository:
                raise ValueError("owner and repository must be non-empty path segments")

        if ref is not None and not ref:
            raise ValueError("ref cannot be empty")
        if timeout <= 0:
            raise ValueError("timeout must be positive")

        self.owner = owner
        self.repository = repository
        self.ref = ref
        self.token = token or os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
        self.api_url = api_url.rstrip("/")
        self.timeout = timeout

    @staticmethod
    def _parse_repository(source: str) -> tuple[str, str]:
        value = source.strip().rstrip("/")
        scp_match = re.fullmatch(
            r"(?:[^@/:]+@)?github\.com:([^/]+)/([^/]+)",
            value,
            flags=re.IGNORECASE,
        )
        if scp_match:
            owner, repository = scp_match.groups()
        else:
            parsed = urlparse(value)
            if parsed.scheme or parsed.netloc:
                if (
                    parsed.hostname is None
                    or parsed.hostname.casefold() != "github.com"
                ):
                    raise ValueError(f"Not a GitHub repository: {source!r}")
                parts = [part for part in parsed.path.split("/") if part]
            else:
                parts = value.split("/")
            if len(parts) != 2:
                raise ValueError(
                    "GitHub repository must be an owner/repository pair or URL"
                )
            owner, repository = parts

        if repository.casefold().endswith(".git"):
            repository = repository[:-4]
        if (
            not owner
            or not repository
            or owner in {".", ".."}
            or repository in {".", ".."}
        ):
            raise ValueError(f"Invalid GitHub repository: {source!r}")
        return owner, repository

    @staticmethod
    def _normalize_path(relative_path: str | PathLike[str]) -> str:
        value = os.fspath(relative_path)
        if not isinstance(value, str):
            raise TypeError("relative_path must be a string or string-like path")
        if value.startswith("/"):
            raise ValueError(f"Path must be relative: {relative_path!r}")

        path = PurePosixPath(value)
        if ".." in path.parts:
            raise ValueError(
                f"Path is outside the accessed repository: {relative_path!r}"
            )
        return "" if str(path) == "." else str(path)

    def _contents_url(self, path: str) -> str:
        url = (
            f"{self.api_url}/repos/{quote(self.owner, safe='')}/"
            f"{quote(self.repository, safe='')}/contents"
        )
        if path:
            url += "/" + quote(path, safe="/")
        return url

    def _request(self, path: str, accept: str) -> bytes:
        headers = {
            "Accept": accept,
            "User-Agent": "vibesense",
            "X-GitHub-Api-Version": "2022-11-28",
        }
        if self.token:
            headers["Authorization"] = f"Bearer {self.token}"

        try:
            response = requests.get(
                self._contents_url(path),
                headers=headers,
                params={"ref": self.ref} if self.ref is not None else None,
                timeout=self.timeout,
            )
        except requests.RequestException as error:
            raise ConnectionError(f"Could not contact GitHub: {error}") from error

        if response.ok:
            return response.content

        try:
            payload = response.json()
            detail = payload.get("message", response.reason)
        except requests.JSONDecodeError, AttributeError:
            detail = response.reason

        display_path = path or "."
        if response.status_code == 404:
            raise FileNotFoundError(display_path)
        if response.status_code in {401, 403}:
            raise PermissionError(
                f"GitHub denied access to {self.owner}/{self.repository}: {detail}"
            )
        raise OSError(f"GitHub API request failed ({response.status_code}): {detail}")

    def _metadata(self, path: str) -> dict | list:
        raw = self._request(path, "application/vnd.github+json")
        try:
            metadata = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise OSError("GitHub returned invalid repository metadata") from error
        if not isinstance(metadata, (dict, list)):
            raise OSError("GitHub returned unexpected repository metadata")
        return metadata

    def list_dir(self, relative_path: str | PathLike[str]) -> list[DirEntry]:
        path = self._normalize_path(relative_path)
        metadata = self._metadata(path)
        if not isinstance(metadata, list):
            if metadata.get("type") in {"file", "symlink", "submodule"}:
                raise NotADirectoryError(path or ".")
            raise OSError(f"Not a directory: {path or '.'}")

        result: list[DirEntry] = []
        for item in metadata:
            if not isinstance(item, dict) or not isinstance(item.get("name"), str):
                continue
            if item.get("type") == "dir":
                result.append(DirEntry(item["name"], FileType.DIRECTORY, None))
            elif item.get("type") in {"file", "symlink", "submodule"}:
                size = item.get("size")
                result.append(
                    DirEntry(
                        item["name"],
                        FileType.FILE,
                        size if isinstance(size, int) else None,
                    )
                )
        return sorted(result, key=lambda entry: entry.name)

    def read_file(
        self,
        relative_path: str | PathLike[str],
        offset: int | None = None,
        limit: int | None = None,
    ) -> str:
        if offset is not None and (not isinstance(offset, int) or offset < 0):
            raise ValueError("offset must be a non-negative integer or None")
        if limit is not None and (not isinstance(limit, int) or limit < 0):
            raise ValueError("limit must be a non-negative integer or None")

        path = self._normalize_path(relative_path)
        metadata = self._metadata(path)
        if isinstance(metadata, list) or metadata.get("type") == "dir":
            raise IsADirectoryError(path or ".")
        if metadata.get("type") not in {"file", "symlink", "submodule"}:
            raise OSError(f"Not a regular file: {path or '.'}")

        raw = self._request(path, "application/vnd.github.raw+json")
        text = raw.decode("utf-8")
        start = offset or 0
        return text[start:] if limit is None else text[start : start + limit]
