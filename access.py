from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
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
    def list_dir(self, relative_path) -> list[DirEntry]:
        """List a directory"""
        pass

    @abstractmethod
    def read_file(self, relative_path, offset=None, limit=None) -> str:
        """Read file from start to end (either/both could be `None`)"""
        pass
