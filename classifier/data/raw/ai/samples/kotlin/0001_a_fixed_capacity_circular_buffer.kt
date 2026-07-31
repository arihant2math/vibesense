class CircularBuffer<T>(private val capacity: Int) {
    private val buffer: Array<Any?> = arrayOfNulls(capacity)
    private var head = 0
    private var tail = 0
    private var size = 0

    init {
        require(capacity > 0) { "Capacity must be greater than zero" }
    }

    val isEmpty: Boolean
        get() = size == 0

    val isFull: Boolean
        get() = size == capacity

    val count: Int
        get() = size

    fun enqueue(element: T) {
        require(!isFull) { "Buffer is full" }
        buffer[tail] = element
        tail = (tail + 1) % capacity
        size++
    }

    fun dequeue(): T {
        require(!isEmpty) { "Buffer is empty" }
        @Suppress("UNCHECKED_CAST")
        val element = buffer[head] as T
        buffer[head] = null
        head = (head + 1) % capacity
        size--
        return element
    }

    fun peek(): T {
        require(!isEmpty) { "Buffer is empty" }
        @Suppress("UNCHECKED_CAST")
        return buffer[head] as T
    }

    fun clear() {
        buffer.fill(null)
        head = 0
        tail = 0
        size = 0
    }
}
