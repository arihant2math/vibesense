package di

import (
	"errors"
	"fmt"
	"reflect"
	"sync"
)

var (
	ErrNotFound     = errors.New("dependency not found")
	ErrInvalidValue = errors.New("invalid value")
)

type Container struct {
	mu       sync.RWMutex
	services map[reflect.Type]*service
}

type service struct {
	once sync.Once
	fn   func(*Container) (any, error)
	val  any
	err  error
}

func New() *Container {
	return &Container{
		services: make(map[reflect.Type]*service),
	}
}

func (c *Container) Provide(constructor any) error {
	t := reflect.TypeOf(constructor)
	if t == nil || t.Kind() != reflect.Func {
		return fmt.Errorf("%w: constructor must be a function", ErrInvalidValue)
	}
	if t.NumOut() < 1 || t.NumOut() > 2 {
		return fmt.Errorf("%w: constructor must return a value and optionally an error", ErrInvalidValue)
	}
	if t.NumOut() == 2 && !isErrorType(t.Out(1)) {
		return fmt.Errorf("%w: second return value must be error", ErrInvalidValue)
	}

	resultType := t.Out(0)
	fnValue := reflect.ValueOf(constructor)

	s := &service{
		fn: func(c *Container) (any, error) {
			args := make([]reflect.Value, t.NumIn())
			for i := range args {
				v, err := c.resolve(t.In(i))
				if err != nil {
					return nil, fmt.Errorf("resolve %v: %w", t.In(i), err)
				}
				args[i] = v
			}

			out := fnValue.Call(args)
			if len(out) == 2 && !out[1].IsNil() {
				return nil, out[1].Interface().(error)
			}
			return out[0].Interface(), nil
		},
	}

	c.mu.Lock()
	if c.services == nil {
		c.services = make(map[reflect.Type]*service)
	}
	c.services[resultType] = s
	c.mu.Unlock()

	return nil
}

func (c *Container) ProvideValue(value any) error {
	if value == nil {
		return ErrInvalidValue
	}
	t := reflect.TypeOf(value)
	c.mu.Lock()
	if c.services == nil {
		c.services = make(map[reflect.Type]*service)
	}
	c.services[t] = &service{
		fn: func(*Container) (any, error) {
			return value, nil
		},
	}
	c.mu.Unlock()
	return nil
}

func Resolve[T any](c *Container) (T, error) {
	var zero T
	t := reflect.TypeOf((*T)(nil)).Elem()

	v, err := c.resolve(t)
	if err != nil {
		return zero, err
	}

	result, ok := v.(T)
	if !ok {
		return zero, fmt.Errorf("resolved value is not %v", t)
	}
	return result, nil
}

func (c *Container) resolve(t reflect.Type) (any, error) {
	c.mu.RLock()
	s := c.services[t]
	c.mu.RUnlock()

	if s == nil {
		return nil, fmt.Errorf("%w: %v", ErrNotFound, t)
	}

	s.once.Do(func() {
		s.val, s.err = s.fn(c)
	})

	if s.err != nil {
		return nil, s.err
	}
	return s.val, nil
}

func isErrorType(t reflect.Type) bool {
	return t.Implements(reflect.TypeOf((*error)(nil)).Elem())
}
