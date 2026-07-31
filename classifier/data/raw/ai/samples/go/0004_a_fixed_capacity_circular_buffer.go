package main

import "fmt"

type CircularBuffer[T any] struct {
	data        []T
	head, tail  int
	size        int
}

func NewCircularBuffer[T any](capacity int) *CircularBuffer[T] {
	if capacity <= 0 {
		panic("capacity must be positive")
	}
	return &CircularBuffer[T]{data: make([]T, capacity)}
}

func (b *CircularBuffer[T]) Enqueue(value T) bool {
	if b.size == len(b.data) {
		return false
	}
	b.data[b.tail] = value
	b.tail = (b.tail + 1) % len(b.data)
	b.size++
	return true
}

func (b *CircularBuffer[T]) Dequeue() (T, bool) {
	if b.size == 0 {
		var zero T
		return zero, false
	}
	value := b.data[b.head]
	var zero T
	b.data[b.head] = zero
	b.head = (b.head + 1) % len(b.data)
	b.size--
	return value, true
}

func (b *CircularBuffer[T]) Peek() (T, bool) {
	if b.size == 0 {
		var zero T
		return zero, false
	}
	return b.data[b.head], true
}

func (b *CircularBuffer[T]) Len() int {
	return b.size
}

func (b *CircularBuffer[T]) Cap() int {
	return len(b.data)
}

func main() {
	buffer := NewCircularBuffer[int](3)

	fmt.Println("enqueue:", buffer.Enqueue(10))
	fmt.Println("enqueue:", buffer.Enqueue(20))
	fmt.Println("enqueue:", buffer.Enqueue(30))
	fmt.Println("enqueue when full:", buffer.Enqueue(40))

	value, ok := buffer.Dequeue()
	fmt.Println("dequeue:", value, ok)

	fmt.Println("enqueue after dequeue:", buffer.Enqueue(40))

	for buffer.Len() > 0 {
		value, _ := buffer.Dequeue()
		fmt.Println("dequeue:", value)
	}

	_, ok = buffer.Peek()
	fmt.Println("peek empty:", ok)
}
