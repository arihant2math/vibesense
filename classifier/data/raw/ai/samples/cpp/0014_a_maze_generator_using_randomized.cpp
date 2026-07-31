#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

class Maze {
public:
    Maze(int rows, int cols) : rows_(rows), cols_(cols), rng_(std::random_device{}()) {
        grid_.assign(2 * rows_ + 1, std::string(2 * cols_ + 1, '#'));
    }

    void generate() {
        carve(0, 0);
    }

    void print() const {
        for (const auto& row : grid_)
            std::cout << row << '\n';
    }

private:
    int rows_, cols_;
    std::vector<std::string> grid_;
    std::mt19937 rng_;

    void carve(int row, int col) {
        grid_[2 * row + 1][2 * col + 1] = ' ';

        std::vector<std::pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}
        };
        std::shuffle(directions.begin(), directions.end(), rng_);

        for (const auto& [dr, dc] : directions) {
            int nextRow = row + dr;
            int nextCol = col + dc;

            if (nextRow < 0 || nextRow >= rows_ ||
                nextCol < 0 || nextCol >= cols_ ||
                grid_[2 * nextRow + 1][2 * nextCol + 1] == ' ') {
                continue;
            }

            grid_[2 * row + 1 + dr][2 * col + 1 + dc] = ' ';
            carve(nextRow, nextCol);
        }
    }
};

int main() {
    constexpr int rows = 10;
    constexpr int cols = 20;

    Maze maze(rows, cols);
    maze.generate();
    maze.print();

    return 0;
}
