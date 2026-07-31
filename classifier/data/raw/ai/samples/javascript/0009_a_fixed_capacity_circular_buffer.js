class CircularBuffer {
  constructor(capacity) {
    if (!Number.isInteger(capacity) || capacity <= 0) {
      throw new Error("Capacity must be a positive integer");
    }

    this.capacity = capacity;
    this.buffer = new Array(capacity);
    this.head = 0;
    this.tail = 0;
    this.count = 0;
  }

  enqueue(value) {
    if (this.isFull()) {
      throw new Error("Buffer is full");
    }

    this.buffer[this.tail] = value;
    this.tail = (this.tail + 1) % this.capacity;
    this.count++;
  }

  dequeue() {
    if (this.isEmpty()) {
      throw new Error("Buffer is empty");
    }

    const value = this.buffer[this.head];
    this.buffer[this.head] = undefined;
    this.head = (this.head + 1) % this.capacity;
    this.count--;

    return value;
  }

  peek() {
    if (this.isEmpty()) {
      return undefined;
    }

    return this.buffer[this.head];
  }

  isEmpty() {
    return this.count === 0;
  }

  isFull() {
    return this.count === this.capacity;
  }

  size() {
    return this.count;
  }

  clear() {
    this.buffer.fill(undefined);
    this.head = 0;
    this.tail = 0;
    this.count = 0;
  }
}

module.exports = CircularBuffer;
