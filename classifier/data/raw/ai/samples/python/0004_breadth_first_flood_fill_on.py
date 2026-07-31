from collections import deque
from typing import List, Tuple


def flood_fill(bitmap: List[List[int]], start: Tuple[int, int], new_color: int) -> List[List[int]]:
    if not bitmap or not bitmap[0]:
        return bitmap

    rows, cols = len(bitmap), len(bitmap[0])
    row, col = start

    if not (0 <= row < rows and 0 <= col < cols):
        return bitmap

    original_color = bitmap[row][col]
    if original_color == new_color:
        return bitmap

    queue = deque([(row, col)])
    bitmap[row][col] = new_color

    while queue:
        r, c = queue.popleft()

        for nr, nc in ((r - 1, c), (r + 1, c), (r, c - 1), (r, c + 1)):
            if (
                0 <= nr < rows
                and 0 <= nc < cols
                and bitmap[nr][nc] == original_color
            ):
                bitmap[nr][nc] = new_color
                queue.append((nr, nc))

    return bitmap


if __name__ == "__main__":
    bitmap = [
        [1, 1, 0],
        [1, 0, 0],
        [0, 0, 1],
    ]
    print(flood_fill(bitmap, (0, 0), 2))
