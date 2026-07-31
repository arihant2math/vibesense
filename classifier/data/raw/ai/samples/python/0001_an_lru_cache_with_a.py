from collections import OrderedDict
from typing import Generic, Hashable, Optional, TypeVar

K = TypeVar("K", bound=Hashable)
V = TypeVar("V")


class LRUCache(Generic[K, V]):
    def __init__(self, capacity: int):
        if capacity < 0:
            raise ValueError("capacity must be non-negative")
        self.capacity = capacity
        self._cache: OrderedDict[K, V] = OrderedDict()

    def get(self, key: K, default: Optional[V] = None) -> Optional[V]:
        if key not in self._cache:
            return default
        self._cache.move_to_end(key)
        return self._cache[key]

    def put(self, key: K, value: V) -> None:
        if self.capacity == 0:
            return
        if key in self._cache:
            self._cache.move_to_end(key)
        self._cache[key] = value
        if len(self._cache) > self.capacity:
            self._cache.popitem(last=False)

    def remove(self, key: K) -> bool:
        if key not in self._cache:
            return False
        del self._cache[key]
        return True

    def clear(self) -> None:
        self._cache.clear()

    def __contains__(self, key: K) -> bool:
        return key in self._cache

    def __len__(self) -> int:
        return len(self._cache)

    def __bool__(self) -> bool:
        return bool(self._cache)
