package pagination

import "errors"

type Page[T any] struct {
	Items   []T
	HasNext bool
}

type Cursor[T any] struct {
	items    []T
	pageSize int
	position int
}

func NewCursor[T any](items []T, pageSize int) (*Cursor[T], error) {
	if pageSize <= 0 {
		return nil, errors.New("page size must be greater than zero")
	}

	return &Cursor[T]{
		items:    items,
		pageSize: pageSize,
	}, nil
}

func (c *Cursor[T]) HasNext() bool {
	return c.position < len(c.items)
}

func (c *Cursor[T]) Next() (Page[T], bool) {
	if !c.HasNext() {
		return Page[T]{}, false
	}

	end := c.position + c.pageSize
	if end > len(c.items) {
		end = len(c.items)
	}

	page := Page[T]{
		Items:   append([]T(nil), c.items[c.position:end]...),
		HasNext: end < len(c.items),
	}

	c.position = end
	return page, true
}
