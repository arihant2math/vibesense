package pool

import (
	"errors"
	"sync"
)

var ErrPoolClosed = errors.New("thread pool is closed")

type Job func()

type Pool struct {
	jobs chan Job

	mu     sync.Mutex
	closed bool

	workers sync.WaitGroup
}

func New(workers, queueSize int) (*Pool, error) {
	if workers <= 0 {
		return nil, errors.New("workers must be greater than zero")
	}
	if queueSize < 0 {
		return nil, errors.New("queue size cannot be negative")
	}

	p := &Pool{
		jobs: make(chan Job, queueSize),
	}

	p.workers.Add(workers)
	for i := 0; i < workers; i++ {
		go p.worker()
	}

	return p, nil
}

func (p *Pool) worker() {
	defer p.workers.Done()

	for job := range p.jobs {
		if job != nil {
			job()
		}
	}
}

func (p *Pool) Submit(job Job) error {
	if job == nil {
		return errors.New("job cannot be nil")
	}

	p.mu.Lock()
	defer p.mu.Unlock()

	if p.closed {
		return ErrPoolClosed
	}

	p.jobs <- job
	return nil
}

func (p *Pool) Shutdown() {
	p.mu.Lock()
	if !p.closed {
		p.closed = true
		close(p.jobs)
	}
	p.mu.Unlock()

	p.workers.Wait()
}
