/**
 * Conway's Game of Life simulation on a toroidal grid.
 */
export class GameOfLife {
  /**
   * Creates a Game of Life simulation.
   * @param {number} width - Grid width.
   * @param {number} height - Grid height.
   * @param {Iterable<number|string>} [initialCells=[]] - Initial cell indices or coordinates.
   */
  constructor(width, height, initialCells = []) {
    if (!Number.isInteger(width) || width <= 0) {
      throw new TypeError("width must be a positive integer");
    }
    if (!Number.isInteger(height) || height <= 0) {
      throw new TypeError("height must be a positive integer");
    }

    this.width = width;
    this.height = height;
    this.cells = new Set();

    for (const cell of initialCells) {
      if (typeof cell === "number") {
        this.cells.add(this.#normalizeIndex(cell));
      } else if (Array.isArray(cell) && cell.length === 2) {
        this.setCell(cell[0], cell[1], true);
      } else {
        throw new TypeError("initial cells must be indices or [x, y] coordinates");
      }
    }
  }

  /**
   * Advances the simulation by one generation.
   * @returns {GameOfLife} This simulation instance.
   */
  step() {
    const neighborCounts = new Map();

    for (const index of this.cells) {
      const x = index % this.width;
      const y = Math.floor(index / this.width);

      for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
          if (dx === 0 && dy === 0) continue;

          const neighborIndex = this.#index(
            this.#wrap(x + dx, this.width),
            this.#wrap(y + dy, this.height)
          );

          neighborCounts.set(
            neighborIndex,
            (neighborCounts.get(neighborIndex) || 0) + 1
          );
        }
      }
    }

    const nextGeneration = new Set();

    for (const [index, count] of neighborCounts) {
      if (count === 3 || (count === 2 && this.cells.has(index))) {
        nextGeneration.add(index);
      }
    }

    this.cells = nextGeneration;
    return this;
  }

  /**
   * Advances the simulation by multiple generations.
   * @param {number} generations - Number of generations to advance.
   * @returns {GameOfLife} This simulation instance.
   */
  run(generations) {
    if (!Number.isInteger(generations) || generations < 0) {
      throw new TypeError("generations must be a non-negative integer");
    }

    for (let i = 0; i < generations; i++) {
      this.step();
    }

    return this;
  }

  /**
   * Returns whether a cell is alive.
   * @param {number} x - Horizontal coordinate.
   * @param {number} y - Vertical coordinate.
   * @returns {boolean} True if the cell is alive.
   */
  isAlive(x, y) {
    return this.cells.has(
      this.#index(this.#wrap(x, this.width), this.#wrap(y, this.height))
    );
  }

  /**
   * Sets the state of a cell.
   * @param {number} x - Horizontal coordinate.
   * @param {number} y - Vertical coordinate.
   * @param {boolean} alive - Whether the cell should be alive.
   * @returns {GameOfLife} This simulation instance.
   */
  setCell(x, y, alive) {
    if (!Number.isInteger(x) || !Number.isInteger(y)) {
      throw new TypeError("coordinates must be integers");
    }

    const index = this.#index(
      this.#wrap(x, this.width),
      this.#wrap(y, this.height)
    );

    if (alive) {
      this.cells.add(index);
    } else {
      this.cells.delete(index);
    }

    return this;
  }

  /**
   * Returns all live cells as coordinate pairs.
   * @returns {Array<[number, number]>} Live cell coordinates.
   */
  getLiveCells() {
    return [...this.cells].map((index) => [
      index % this.width,
      Math.floor(index / this.width),
    ]);
  }

  /**
   * Clears all live cells from the grid.
   * @returns {GameOfLife} This simulation instance.
   */
  clear() {
    this.cells.clear();
    return this;
  }

  #index(x, y) {
    return y * this.width + x;
  }

  #normalizeIndex(index) {
    if (!Number.isInteger(index) || index < 0 || index >= this.width * this.height) {
      throw new RangeError("cell index is outside the grid");
    }
    return index;
  }

  #wrap(value, size) {
    return ((value % size) + size) % size;
  }
}
