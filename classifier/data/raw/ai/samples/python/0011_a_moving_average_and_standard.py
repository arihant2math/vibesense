from __future__ import annotations

from collections import deque
from math import isfinite, sqrt
from numbers import Real
from typing import Deque, Optional


class MovingStatistics:
    """Rolling moving average and standard deviation accumulator."""

    def __init__(self, window_size: int, *, sample: bool = False) -> None:
        if isinstance(window_size, bool) or not isinstance(window_size, int):
            raise TypeError("window_size must be an integer")
        if window_size <= 0:
            raise ValueError("window_size must be greater than zero")
        if not isinstance(sample, bool):
            raise TypeError("sample must be a boolean")

        self._window_size = window_size
        self._sample = sample
        self._values: Deque[float] = deque(maxlen=window_size)
        self._sum = 0.0
        self._sum_of_squares = 0.0

    @property
    def window_size(self) -> int:
        return self._window_size

    @property
    def count(self) -> int:
        return len(self._values)

    @property
    def average(self) -> float:
        self._require_values()
        return self._sum / self.count

    @property
    def variance(self) -> float:
        self._require_values()

        if self._sample and self.count < 2:
            raise ValueError(
                "sample variance requires at least two observations"
            )

        denominator = self.count - 1 if self._sample else self.count
        variance = (
            self._sum_of_squares - (self._sum * self._sum) / self.count
        ) / denominator

        return max(0.0, variance)

    @property
    def standard_deviation(self) -> float:
        return sqrt(self.variance)

    def add(self, value: Real) -> MovingStatistics:
        numeric_value = self._validate_value(value)

        if len(self._values) == self._window_size:
            removed = self._values.popleft()
            self._sum -= removed
            self._sum_of_squares -= removed * removed

        self._values.append(numeric_value)
        self._sum += numeric_value
        self._sum_of_squares += numeric_value * numeric_value
        return self

    def update(self, values: object) -> MovingStatistics:
        if values is None:
            raise TypeError("values must be an iterable of real numbers")

        try:
            iterator = iter(values)
        except TypeError as exc:
            raise TypeError("values must be an iterable of real numbers") from exc

        for value in iterator:
            self.add(value)

        return self

    def reset(self) -> None:
        self._values.clear()
        self._sum = 0.0
        self._sum_of_squares = 0.0

    def snapshot(self) -> dict[str, Optional[float] | int]:
        if not self._values:
            return {
                "count": 0,
                "average": None,
                "variance": None,
                "standard_deviation": None,
            }

        variance = None
        standard_deviation = None

        if not self._sample or self.count >= 2:
            variance = self.variance
            standard_deviation = sqrt(variance)

        return {
            "count": self.count,
            "average": self.average,
            "variance": variance,
            "standard_deviation": standard_deviation,
        }

    def _require_values(self) -> None:
        if not self._values:
            raise ValueError("no values have been added")

    @staticmethod
    def _validate_value(value: Real) -> float:
        if isinstance(value, bool) or not isinstance(value, Real):
            raise TypeError("value must be a real number")

        numeric_value = float(value)

        if not isfinite(numeric_value):
            raise ValueError("value must be finite")

        return numeric_value


if __name__ == "__main__":
    accumulator = MovingStatistics(window_size=5)
    accumulator.update([1, 2, 3, 4, 5, 6])
    print(accumulator.snapshot())
