#include <stdint.h>
#include <stdlib.h>

typedef enum {
    MAZE_NORTH = 1u << 0,
    MAZE_EAST  = 1u << 1,
    MAZE_SOUTH = 1u << 2,
    MAZE_WEST  = 1u << 3
} MazeDirection;

typedef unsigned (*MazeRandomFn)(void *context, unsigned upper_bound);

typedef struct {
    size_t width;
    size_t height;
    uint8_t *walls;
} Maze;

int maze_init(Maze *maze, size_t width, size_t height);
void maze_free(Maze *maze);
int maze_generate(Maze *maze, MazeRandomFn random_fn, void *random_context);
int maze_has_wall(const Maze *maze, size_t x, size_t y, MazeDirection direction);
int maze_is_valid(const Maze *maze);

static unsigned maze_default_random(void *context, unsigned upper_bound)
{
    (void)context;
    return upper_bound == 0 ? 0 : (unsigned)((unsigned long)rand() % upper_bound);
}

static size_t maze_index(const Maze *maze, size_t x, size_t y)
{
    return y * maze->width + x;
}

static MazeDirection maze_opposite(MazeDirection direction)
{
    switch (direction) {
        case MAZE_NORTH: return MAZE_SOUTH;
        case MAZE_EAST:  return MAZE_WEST;
        case MAZE_SOUTH: return MAZE_NORTH;
        case MAZE_WEST:  return MAZE_EAST;
        default:         return 0;
    }
}

static void maze_step(
    size_t x,
    size_t y,
    MazeDirection direction,
    size_t *next_x,
    size_t *next_y
) {
    *next_x = x;
    *next_y = y;

    switch (direction) {
        case MAZE_NORTH: --*next_y; break;
        case MAZE_EAST:  ++*next_x; break;
        case MAZE_SOUTH: ++*next_y; break;
        case MAZE_WEST:  --*next_x; break;
        default: break;
    }
}

int maze_init(Maze *maze, size_t width, size_t height)
{
    size_t cell_count;

    if (maze == NULL || width == 0 || height == 0) {
        return 0;
    }

    if (width > SIZE_MAX / height) {
        return 0;
    }

    cell_count = width * height;

    maze->walls = (uint8_t *)malloc(cell_count * sizeof(*maze->walls));
    if (maze->walls == NULL) {
        maze->width = 0;
        maze->height = 0;
        return 0;
    }

    maze->width = width;
    maze->height = height;

    for (size_t i = 0; i < cell_count; ++i) {
        maze->walls[i] = MAZE_NORTH | MAZE_EAST | MAZE_SOUTH | MAZE_WEST;
    }

    return 1;
}

void maze_free(Maze *maze)
{
    if (maze == NULL) {
        return;
    }

    free(maze->walls);
    maze->walls = NULL;
    maze->width = 0;
    maze->height = 0;
}

int maze_generate(Maze *maze, MazeRandomFn random_fn, void *random_context)
{
    size_t cell_count;
    size_t *stack;
    uint8_t *visited;

    if (!maze_is_valid(maze)) {
        return 0;
    }

    if (random_fn == NULL) {
        random_fn = maze_default_random;
    }

    cell_count = maze->width * maze->height;
    stack = (size_t *)malloc(cell_count * sizeof(*stack));
    visited = (uint8_t *)calloc(cell_count, sizeof(*visited));

    if (stack == NULL || visited == NULL) {
        free(stack);
        free(visited);
        return 0;
    }

    for (size_t i = 0; i < cell_count; ++i) {
        maze->walls[i] = MAZE_NORTH | MAZE_EAST | MAZE_SOUTH | MAZE_WEST;
    }

    size_t stack_size = 1;
    stack[0] = 0;
    visited[0] = 1;

    while (stack_size > 0) {
        size_t current = stack[stack_size - 1];
        size_t x = current % maze->width;
        size_t y = current / maze->width;
        MazeDirection directions[4] = {
            MAZE_NORTH, MAZE_EAST, MAZE_SOUTH, MAZE_WEST
        };
        size_t unvisited_count = 0;

        for (size_t i = 0; i < 4; ++i) {
            size_t next_x;
            size_t next_y;

            maze_step(x, y, directions[i], &next_x, &next_y);

            if (next_x < maze->width &&
                next_y < maze->height &&
                !visited[maze_index(maze, next_x, next_y)]) {
                ++unvisited_count;
            }
        }

        if (unvisited_count == 0) {
            --stack_size;
            continue;
        }

        for (size_t i = 3; i > 0; --i) {
            size_t j = random_fn(random_context, (unsigned)(i + 1));
            MazeDirection temporary = directions[i];
            directions[i] = directions[j];
            directions[j] = temporary;
        }

        for (size_t i = 0; i < 4; ++i) {
            size_t next_x;
            size_t next_y;
            size_t next_index;
            MazeDirection direction = directions[i];

            maze_step(x, y, direction, &next_x, &next_y);

            if (next_x >= maze->width || next_y >= maze->height) {
                continue;
            }

            next_index = maze_index(maze, next_x, next_y);

            if (visited[next_index]) {
                continue;
            }

            maze->walls[current] &= (uint8_t)~direction;
            maze->walls[next_index] &= (uint8_t)~maze_opposite(direction);
            visited[next_index] = 1;
            stack[stack_size++] = next_index;
            break;
        }
    }

    free(stack);
    free(visited);
    return 1;
}

int maze_has_wall(
    const Maze *maze,
    size_t x,
    size_t y,
    MazeDirection direction
) {
    if (!maze_is_valid(maze) ||
        x >= maze->width ||
        y >= maze->height ||
        (direction & (MAZE_NORTH | MAZE_EAST | MAZE_SOUTH | MAZE_WEST)) == 0) {
        return -1;
    }

    return (maze->walls[maze_index(maze, x, y)] & direction) != 0;
}

int maze_is_valid(const Maze *maze)
{
    return maze != NULL &&
           maze->walls != NULL &&
           maze->width > 0 &&
           maze->height > 0;
}
