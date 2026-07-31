/**
 * Performs a breadth-first flood fill on a bitmap using four-way connectivity.
 *
 * @param {Array<Array<*>>} bitmap - A rectangular bitmap represented as rows.
 * @param {number} startX - The horizontal coordinate of the starting pixel.
 * @param {number} startY - The vertical coordinate of the starting pixel.
 * @param {*} replacementColor - The color to apply to the connected region.
 * @returns {Array<Array<*>>} The modified bitmap.
 * @throws {TypeError} If the bitmap is not a rectangular array.
 * @throws {RangeError} If the starting coordinate is outside the bitmap.
 */
export function floodFill(bitmap, startX, startY, replacementColor) {
  validateBitmap(bitmap);

  const height = bitmap.length;
  const width = bitmap[0].length;

  if (
    !Number.isInteger(startX) ||
    !Number.isInteger(startY) ||
    startX < 0 ||
    startX >= width ||
    startY < 0 ||
    startY >= height
  ) {
    throw new RangeError("Starting coordinates are outside the bitmap.");
  }

  const targetColor = bitmap[startY][startX];

  if (Object.is(targetColor, replacementColor)) {
    return bitmap;
  }

  const queue = [[startX, startY]];
  let head = 0;

  bitmap[startY][startX] = replacementColor;

  while (head < queue.length) {
    const [x, y] = queue[head++];

    visitNeighbor(x - 1, y);
    visitNeighbor(x + 1, y);
    visitNeighbor(x, y - 1);
    visitNeighbor(x, y + 1);
  }

  return bitmap;

  function visitNeighbor(x, y) {
    if (
      x >= 0 &&
      x < width &&
      y >= 0 &&
      y < height &&
      Object.is(bitmap[y][x], targetColor)
    ) {
      bitmap[y][x] = replacementColor;
      queue.push([x, y]);
    }
  }
}

function validateBitmap(bitmap) {
  if (!Array.isArray(bitmap) || bitmap.length === 0) {
    throw new TypeError("Bitmap must be a non-empty array of rows.");
  }

  if (!Array.isArray(bitmap[0]) || bitmap[0].length === 0) {
    throw new TypeError("Bitmap rows must be non-empty arrays.");
  }

  const width = bitmap[0].length;

  for (const row of bitmap) {
    if (!Array.isArray(row) || row.length !== width) {
      throw new TypeError("Bitmap must be a rectangular array.");
    }
  }
}
