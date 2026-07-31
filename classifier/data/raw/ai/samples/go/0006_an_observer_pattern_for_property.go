package propertyobserver

import (
	"errors"
	"fmt"
	"reflect"
	"sync"
)

var (
	ErrNilObservable   = errors.New("observable is nil")
	ErrEmptyProperty   = errors.New("property name is empty")
	ErrObserverNil     = errors.New("observer is nil")
	ErrSubscriptionNil = errors.New("subscription is nil")
)

type Observer func(property string, oldValue, newValue any) error

type Subscription struct {
	owner *Observable
	id    uint64
	once  sync.Once
}

func (s *Subscription) Unsubscribe() error {
	if s == nil {
		return ErrSubscriptionNil
	}

	var err error
	s.once.Do(func() {
		if s.owner == nil {
			err = ErrNilObservable
			return
		}
		err = s.owner.unsubscribe(s.id)
	})
	return err
}

type Observable struct {
	mu          sync.RWMutex
	values      map[string]any
	observers   map[string]map[uint64]Observer
	nextID      uint64
	initialized bool
}

func New() *Observable {
	return &Observable{
		values:    make(map[string]any),
		observers: make(map[string]map[uint64]Observer),
	}
}

func (o *Observable) Set(property string, value any) error {
	if o == nil {
		return ErrNilObservable
	}
	if err := validateProperty(property); err != nil {
		return err
	}

	o.mu.Lock()
	if !o.initialized {
		o.values = make(map[string]any)
		o.observers = make(map[string]map[uint64]Observer)
		o.initialized = true
	}

	oldValue, exists := o.values[property]
	if exists && reflect.DeepEqual(oldValue, value) {
		o.mu.Unlock()
		return nil
	}

	o.values[property] = value
	callbacks := make([]Observer, 0, len(o.observers[property]))
	for _, observer := range o.observers[property] {
		callbacks = append(callbacks, observer)
	}
	o.mu.Unlock()

	var errs []error
	for _, observer := range callbacks {
		if observer == nil {
			continue
		}
		if err := invokeObserver(observer, property, oldValue, value); err != nil {
			errs = append(errs, err)
		}
	}

	return errors.Join(errs...)
}

func (o *Observable) Get(property string) (any, bool, error) {
	if o == nil {
		return nil, false, ErrNilObservable
	}
	if err := validateProperty(property); err != nil {
		return nil, false, err
	}

	o.mu.RLock()
	value, exists := o.values[property]
	o.mu.RUnlock()
	return value, exists, nil
}

func (o *Observable) Observe(property string, observer Observer) (*Subscription, error) {
	if o == nil {
		return nil, ErrNilObservable
	}
	if err := validateProperty(property); err != nil {
		return nil, err
	}
	if isNilObserver(observer) {
		return nil, ErrObserverNil
	}

	o.mu.Lock()
	if !o.initialized {
		o.values = make(map[string]any)
		o.observers = make(map[string]map[uint64]Observer)
		o.initialized = true
	}
	if o.observers[property] == nil {
		o.observers[property] = make(map[uint64]Observer)
	}
	o.nextID++
	if o.nextID == 0 {
		o.nextID++
	}
	id := o.nextID
	o.observers[property][id] = observer
	o.mu.Unlock()

	return &Subscription{owner: o, id: id}, nil
}

func (o *Observable) unsubscribe(id uint64) error {
	if o == nil {
		return ErrNilObservable
	}
	if id == 0 {
		return errors.New("invalid subscription ID")
	}

	o.mu.Lock()
	defer o.mu.Unlock()

	for property, observers := range o.observers {
		if _, exists := observers[id]; exists {
			delete(observers, id)
			if len(observers) == 0 {
				delete(o.observers, property)
			}
			return nil
		}
	}

	return fmt.Errorf("subscription %d not found", id)
}

func validateProperty(property string) error {
	if property == "" {
		return ErrEmptyProperty
	}
	if len([]rune(property)) > 1024 {
		return errors.New("property name exceeds 1024 characters")
	}
	return nil
}

func isNilObserver(observer Observer) bool {
	if observer == nil {
		return true
	}
	value := reflect.ValueOf(observer)
	return value.Kind() == reflect.Func && value.IsNil()
}

func invokeObserver(observer Observer, property string, oldValue, newValue any) (err error) {
	defer func() {
		if recovered := recover(); recovered != nil {
			err = fmt.Errorf("observer panic: %v", recovered)
		}
	}()
	return observer(property, oldValue, newValue)
}
