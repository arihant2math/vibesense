from typing import Iterable, List, Sequence


def game_of_life(
    grid: Sequence[Sequence[int]],
    generations: int = 1,
) -> List[List[int]]:
    if generations < 0:
        raise ValueError("generations must be non-negative")

    if not grid:
        return []

    rows = len(grid)
    cols = len(grid[0]) if grid[0] else 0

    if cols == 0:
        return [[] for _ in range(rows)]

    if any(len(row) != cols for row in grid):
        raise ValueError("grid must be rectangular")

    current = [[1 if cell else 0 for cell in row] for row in grid]

    for _ in range(generations):
        next_grid = [[0] * cols for _ in range(rows)]

        for row in range(rows):
            for col in range(cols):
                neighbors = 0

                for row_offset in (-1, 0, 1):
                    for col_offset in (-1, 0, 1):
                        if row_offset == 0 and col_offset == 0:
                            continue

                        neighbors += current[
                            (row + row_offset) % rows
                        ][(col + col_offset) % cols]

                if current[row][col]:
                    next_grid[row][col] = int(neighbors in (2, 3))
                else:
                    next_grid[row][col] = int(neighbors == 3)

        current = next_grid

    return current


def simulate(
    grid: Iterable[Iterable[int]],
    generations: int = 1,
) -> List[List[int]]:
    return game_of_life([list(row) for row in grid], generations)
