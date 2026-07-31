from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Generic, TypeVar


T = TypeVar("T")
Observer = Callable[["PropertyChange[T]"], None]


@dataclass(frozen=True)
class PropertyChange(Generic[T]):
    source: Any
    name: str
    old_value: T
    new_value: T


class Observable:
    def __init__(self) -> None:
        self._observers: dict[str, list[Observer[Any]]] = {}

    def observe(self, property_name: str, observer: Observer[Any]) -> Callable[[], None]:
        observers = self._observers.setdefault(property_name, [])
        observers.append(observer)

        def unsubscribe() -> None:
            if observer in observers:
                observers.remove(observer)

        return unsubscribe

    def _notify(self, name: str, old_value: Any, new_value: Any) -> None:
        if old_value == new_value:
            return

        change = PropertyChange(
            source=self,
            name=name,
            old_value=old_value,
            new_value=new_value,
        )

        for observer in tuple(self._observers.get(name, ())):
            observer(change)


class Person(Observable):
    def __init__(self, name: str) -> None:
        super().__init__()
        self._name = name

    @property
    def name(self) -> str:
        return self._name

    @name.setter
    def name(self, value: str) -> None:
        old_value = self._name
        self._name = value
        self._notify("name", old_value, value)
