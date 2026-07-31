from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from os import PathLike
from pathlib import Path
from typing import Optional


class FileType(Enum):
    FILE = "file"
    DIRECTORY = "dir"


@dataclass
class DirEntry:
    name: str
    entry_type: FileType
    size: Optional[int]


class Accessor(ABC):
    @abstractmethod
    def list_dir(self, relative_path: str | PathLike[str]) -> list[DirEntry]:
        """List a directory"""
        pass

    @abstractmethod
    def read_file(
        self, relative_path: str | PathLike[str], offset=None, limit=None
    ) -> str:
        """Read file from start to end (either/both could be `None`)"""
        pass


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
