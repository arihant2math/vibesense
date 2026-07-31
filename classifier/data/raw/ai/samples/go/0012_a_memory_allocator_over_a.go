package fixedarena

import (
	"errors"
	"sort"
	"sync"
)

var (
	ErrOutOfMemory      = errors.New("allocator: out of memory")
	ErrInvalidBlock     = errors.New("allocator: invalid block")
	ErrDoubleFree       = errors.New("allocator: block already freed")
	ErrInvalidArenaSize = errors.New("allocator: arena size must be positive")
)

type block struct {
	offset int
	size   int
}

type Allocator struct {
	mu          sync.Mutex
	arena       []byte
	free        []block
	allocations map[int]int
}

func New(arena []byte) (*Allocator, error) {
	if len(arena) == 0 {
		return nil, ErrInvalidArenaSize
	}

	return &Allocator{
		arena: arena,
		free: []block{
			{offset: 0, size: len(arena)},
		},
		allocations: make(map[int]int),
	}, nil
}

func (a *Allocator) Allocate(size int) ([]byte, error) {
	if size <= 0 {
		return nil, ErrInvalidBlock
	}

	a.mu.Lock()
	defer a.mu.Unlock()

	for i, available := range a.free {
		if available.size < size {
			continue
		}

		offset := available.offset
		if available.size == size {
			a.free = append(a.free[:i], a.free[i+1:]...)
		} else {
			a.free[i] = block{
				offset: available.offset + size,
				size:   available.size - size,
			}
		}

		a.allocations[offset] = size
		return a.arena[offset : offset+size], nil
	}

	return nil, ErrOutOfMemory
}

func (a *Allocator) Free(data []byte) error {
	if len(data) == 0 {
		return ErrInvalidBlock
	}

	a.mu.Lock()
	defer a.mu.Unlock()

	offset := -1
	for candidate, size := range a.allocations {
		if size == len(data) && &a.arena[candidate] == &data[0] {
			offset = candidate
			break
		}
	}

	if offset < 0 {
		return ErrInvalidBlock
	}

	size := a.allocations[offset]
	delete(a.allocations, offset)

	a.free = append(a.free, block{
		offset: offset,
		size:   size,
	})
	a.mergeFreeBlocks()

	return nil
}

func (a *Allocator) Reset() {
	a.mu.Lock()
	defer a.mu.Unlock()

	a.free = []block{
		{offset: 0, size: len(a.arena)},
	}
	a.allocations = make(map[int]int)
}

func (a *Allocator) Capacity() int {
	a.mu.Lock()
	defer a.mu.Unlock()

	return len(a.arena)
}

func (a *Allocator) Used() int {
	a.mu.Lock()
	defer a.mu.Unlock()

	used := 0
	for _, size := range a.allocations {
		used += size
	}
	return used
}

func (a *Allocator) mergeFreeBlocks() {
	sort.Slice(a.free, func(i, j int) bool {
		return a.free[i].offset < a.free[j].offset
	})

	merged := make([]block, 0, len(a.free))
	for _, current := range a.free {
		if len(merged) == 0 {
			merged = append(merged, current)
			continue
		}

		last := &merged[len(merged)-1]
		if last.offset+last.size == current.offset {
			last.size += current.size
		} else {
			merged = append(merged, current)
		}
	}

	a.free = merged
}
