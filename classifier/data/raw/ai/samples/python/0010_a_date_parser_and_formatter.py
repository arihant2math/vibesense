from __future__ import annotations

from datetime import datetime, timedelta, timezone
from typing import Optional


def parse_iso8601(value: str) -> datetime:
    """Parse an ISO 8601 timestamp into a timezone-aware datetime."""
    length = len(value)
    if length < 10:
        raise ValueError("Invalid ISO 8601 timestamp")

    try:
        year = int(value[0:4])
        if value[4] != "-":
            raise ValueError
        month = int(value[5:7])
        if value[7] != "-":
            raise ValueError
        day = int(value[8:10])
        position = 10

        if position == length:
            return datetime(year, month, day)

        if value[position] in ("T", "t", " "):
            position += 1
        else:
            raise ValueError

        hour = int(value[position:position + 2])
        position += 2
        if position >= length or value[position] != ":":
            raise ValueError
        position += 1

        minute = int(value[position:position + 2])
        position += 2
        second = 0
        microsecond = 0

        if position < length and value[position] == ":":
            position += 1
            second = int(value[position:position + 2])
            position += 2

            if position < length and value[position] == ".":
                position += 1
                start = position
                while position < length and "0" <= value[position] <= "9":
                    position += 1
                digits = position - start
                if digits == 0:
                    raise ValueError
                microsecond = int(value[start:start + min(digits, 6)].ljust(6, "0"))

        tz = None
        if position < length:
            marker = value[position]
            if marker in ("Z", "z"):
                tz = timezone.utc
                position += 1
            elif marker in ("+", "-"):
                sign = 1 if marker == "+" else -1
                position += 1
                offset_hour = int(value[position:position + 2])
                position += 2
                offset_minute = 0
                if position < length and value[position] == ":":
                    position += 1
                    offset_minute = int(value[position:position + 2])
                    position += 2
                elif position + 2 <= length:
                    offset_minute = int(value[position:position + 2])
                    position += 2
                tz = timezone(sign * timedelta(hours=offset_hour,
                                               minutes=offset_minute))

        if position != length:
            raise ValueError

        return datetime(
            year, month, day, hour, minute, second, microsecond, tz
        )
    except (TypeError, ValueError, IndexError):
        raise ValueError(f"Invalid ISO 8601 timestamp: {value!r}") from None


def format_iso8601(
    value: datetime,
    *,
    use_z: bool = True,
    timespec: str = "auto",
) -> str:
    """Format a datetime as an ISO 8601 timestamp."""
    if value.tzinfo is None:
        return value.isoformat(timespec=timespec)

    result = value.isoformat(timespec=timespec)
    if use_z and value.utcoffset() == timedelta(0):
        result = result[:-6] + "Z"
    return result


def parse_and_format(
    value: str,
    *,
    use_z: bool = True,
    timespec: str = "auto",
) -> str:
    return format_iso8601(parse_iso8601(value), use_z=use_z, timespec=timespec)
