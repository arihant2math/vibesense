from typing import Any, Callable, List, Optional, TypeVar

T = TypeVar("T")
Comparator = Callable[[T, T], int]


def merge_sort(items: List[T], compare: Optional[Comparator[T]] = None) -> List[T]:
    compare = compare or _default_compare

    if len(items) <= 1:
        return items.copy()

    middle = len(items) // 2
    left = merge_sort(items[:middle], compare)
    right = merge_sort(items[middle:], compare)

    merged: List[T] = []
    left_index = right_index = 0

    while left_index < len(left) and right_index < len(right):
        if compare(left[left_index], right[right_index]) <= 0:
            merged.append(left[left_index])
            left_index += 1
        else:
            merged.append(right[right_index])
            right_index += 1

    merged.extend(left[left_index:])
    merged.extend(right[right_index:])
    return merged


def quicksort(items: List[T], compare: Optional[Comparator[T]] = None) -> List[T]:
    compare = compare or _default_compare

    if len(items) <= 1:
        return items.copy()

    pivot = items[len(items) // 2]
    less = [item for item in items if compare(item, pivot) < 0]
    equal = [item for item in items if compare(item, pivot) == 0]
    greater = [item for item in items if compare(item, pivot) > 0]

    return quicksort(less, compare) + equal + quicksort(greater, compare)


def _default_compare(left: T, right: T) -> int:
    if left < right:
        return -1
    if left > right:
        return 1
    return 0


if __name__ == "__main__":
    values = [5, 2, 9, 1, 5, 6]
    print(merge_sort(values))
    print(quicksort(values))
