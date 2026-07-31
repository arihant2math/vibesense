'use strict';

function floodFill(bitmap, startX, startY, replacementColor) {
  validateBitmap(bitmap);

  const height = bitmap.length;
  const width = bitmap[0].length;

  if (!Number.isInteger(startX) || !Number.isInteger(startY)) {
    throw new TypeError('Start coordinates must be integers');
  }

  if (
    startX < 0 ||
    startX >= width ||
    startY < 0 ||
    startY >= height
  ) {
    return bitmap;
  }

  const targetColor = bitmap[startY][startX];

  if (Object.is(targetColor, replacementColor)) {
    return bitmap;
  }

  const queue = [[startX, startY]];
  let head = 0;

  while (head < queue.length) {
    const [x, y] = queue[head++];

    if (!Object.is(bitmap[y][x], targetColor)) {
      continue;
    }

    bitmap[y][x] = replacementColor;

    if (x > 0) queue.push([x - 1, y]);
    if (x < width - 1) queue.push([x + 1, y]);
    if (y > 0) queue.push([x, y - 1]);
    if (y < height - 1) queue.push([x, y + 1]);
  }

  return bitmap;
}

function validateBitmap(bitmap) {
  if (!Array.isArray(bitmap) || bitmap.length === 0) {
    throw new TypeError('Bitmap must be a non-empty 2D array');
  }

  if (!Array.isArray(bitmap[0]) || bitmap[0].length === 0) {
    throw new TypeError('Bitmap rows must be non-empty arrays');
  }

  const width = bitmap[0].length;

  for (const row of bitmap) {
    if (!Array.isArray(row) || row.length !== width) {
      throw new TypeError('Bitmap must be rectangular');
    }
  }
}

module.exports = { floodFill };
