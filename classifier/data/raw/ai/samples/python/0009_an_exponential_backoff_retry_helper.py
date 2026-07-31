import random
import time
from collections.abc import Callable
from typing import TypeVar

T = TypeVar("T")


def retry_with_exponential_backoff(
    operation: Callable[[], T],
    *,
    max_attempts: int = 5,
    initial_delay: float = 1.0,
    max_delay: float = 30.0,
    jitter: float = 0.1,
) -> T:
    """Retry an operation with exponential backoff and random jitter."""

    if max_attempts < 1:
        raise ValueError("max_attempts must be at least 1")
    if initial_delay < 0:
        raise ValueError("initial_delay must not be negative")
    if max_delay < initial_delay:
        raise ValueError("max_delay must be at least initial_delay")
    if jitter < 0:
        raise ValueError("jitter must not be negative")

    for attempt in range(max_attempts):
        try:
            return operation()
        except Exception:
            if attempt == max_attempts - 1:
                raise

            delay = min(initial_delay * (2**attempt), max_delay)
            jitter_amount = random.uniform(0, delay * jitter)
            time.sleep(delay + jitter_amount)

    raise RuntimeError("Retry loop completed unexpectedly")
