import java.util.Arrays;

public class GameOfLife {
    private final int rows;
    private final int cols;
    private boolean[][] grid;

    public GameOfLife(int rows, int cols) {
        if (rows <= 0 || cols <= 0) {
            throw new IllegalArgumentException("Grid dimensions must be positive");
        }
        this.rows = rows;
        this.cols = cols;
        this.grid = new boolean[rows][cols];
    }

    public void setCell(int row, int col, boolean alive) {
        grid[Math.floorMod(row, rows)][Math.floorMod(col, cols)] = alive;
    }

    public boolean isAlive(int row, int col) {
        return grid[Math.floorMod(row, rows)][Math.floorMod(col, cols)];
    }

    public void step() {
        boolean[][] next = new boolean[rows][cols];

        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                int neighbors = countNeighbors(row, col);
                next[row][col] = neighbors == 3 || (grid[row][col] && neighbors == 2);
            }
        }

        grid = next;
    }

    private int countNeighbors(int row, int col) {
        int count = 0;

        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr != 0 || dc != 0) {
                    int r = Math.floorMod(row + dr, rows);
                    int c = Math.floorMod(col + dc, cols);
                    if (grid[r][c]) {
                        count++;
                    }
                }
            }
        }

        return count;
    }

    public void clear() {
        for (boolean[] row : grid) {
            Arrays.fill(row, false);
        }
    }

    @Override
    public String toString() {
        StringBuilder output = new StringBuilder();

        for (boolean[] row : grid) {
            for (boolean cell : row) {
                output.append(cell ? '█' : ' ');
            }
            output.append(System.lineSeparator());
        }

        return output.toString();
    }

    public static void main(String[] args) throws InterruptedException {
        GameOfLife life = new GameOfLife(20, 40);

        life.setCell(1, 2, true);
        life.setCell(2, 3, true);
        life.setCell(3, 1, true);
        life.setCell(3, 2, true);
        life.setCell(3, 3, true);

        for (int generation = 0; generation < 100; generation++) {
            System.out.print("\033[H\033[2J");
            System.out.flush();
            System.out.println(life);
            life.step();
            Thread.sleep(100);
        }
    }
}
