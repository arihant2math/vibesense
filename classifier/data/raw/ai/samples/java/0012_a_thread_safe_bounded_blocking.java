import java.util.ArrayDeque;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

/**
 * A thread-safe, bounded, blocking FIFO queue.
 *
 * @param <E> the element type
 */
public class BoundedBlockingQueue<E> {
    private final ArrayDeque<E> elements;
    private final int capacity;
    private final ReentrantLock lock = new ReentrantLock();
    private final Condition notEmpty = lock.newCondition();
    private final Condition notFull = lock.newCondition();

    /**
     * Creates a queue with the specified maximum capacity.
     *
     * @param capacity the maximum number of elements
     * @throws IllegalArgumentException if capacity is less than one
     */
    public BoundedBlockingQueue(int capacity) {
        if (capacity < 1) {
            throw new IllegalArgumentException("Capacity must be at least 1");
        }
        this.capacity = capacity;
        this.elements = new ArrayDeque<>(capacity);
    }

    /**
     * Adds an element, waiting until space becomes available if necessary.
     *
     * @param element the element to add
     * @throws InterruptedException if interrupted while waiting
     * @throws NullPointerException if element is null
     */
    public void put(E element) throws InterruptedException {
        Objects.requireNonNull(element, "element");
        lock.lockInterruptibly();
        try {
            while (elements.size() == capacity) {
                notFull.await();
            }
            elements.addLast(element);
            notEmpty.signal();
        } finally {
            lock.unlock();
        }
    }

    /**
     * Removes and returns the head element, waiting until an element is available.
     *
     * @return the head element
     * @throws InterruptedException if interrupted while waiting
     */
    public E take() throws InterruptedException {
        lock.lockInterruptibly();
        try {
            while (elements.isEmpty()) {
                notEmpty.await();
            }
            E element = elements.removeFirst();
            notFull.signal();
            return element;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Attempts to add an element without waiting.
     *
     * @param element the element to add
     * @return true if the element was added, or false if the queue is full
     * @throws NullPointerException if element is null
     */
    public boolean offer(E element) {
        Objects.requireNonNull(element, "element");
        lock.lock();
        try {
            if (elements.size() == capacity) {
                return false;
            }
            elements.addLast(element);
            notEmpty.signal();
            return true;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Attempts to add an element, waiting up to the specified duration for space.
     *
     * @param element the element to add
     * @param timeout the maximum time to wait
     * @param unit the timeout unit
     * @return true if the element was added, or false if the timeout elapsed
     * @throws InterruptedException if interrupted while waiting
     * @throws NullPointerException if element or unit is null
     */
    public boolean offer(E element, long timeout, TimeUnit unit)
            throws InterruptedException {
        Objects.requireNonNull(element, "element");
        Objects.requireNonNull(unit, "unit");

        long nanos = unit.toNanos(timeout);
        lock.lockInterruptibly();
        try {
            while (elements.size() == capacity) {
                if (nanos <= 0L) {
                    return false;
                }
                nanos = notFull.awaitNanos(nanos);
            }
            elements.addLast(element);
            notEmpty.signal();
            return true;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Attempts to remove and return the head element without waiting.
     *
     * @return the head element, or null if the queue is empty
     */
    public E poll() {
        lock.lock();
        try {
            if (elements.isEmpty()) {
                return null;
            }
            E element = elements.removeFirst();
            notFull.signal();
            return element;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Attempts to remove and return the head element, waiting up to the specified duration.
     *
     * @param timeout the maximum time to wait
     * @param unit the timeout unit
     * @return the head element, or null if the timeout elapsed
     * @throws InterruptedException if interrupted while waiting
     * @throws NullPointerException if unit is null
     */
    public E poll(long timeout, TimeUnit unit) throws InterruptedException {
        Objects.requireNonNull(unit, "unit");

        long nanos = unit.toNanos(timeout);
        lock.lockInterruptibly();
        try {
            while (elements.isEmpty()) {
                if (nanos <= 0L) {
                    return null;
                }
                nanos = notEmpty.awaitNanos(nanos);
            }
            E element = elements.removeFirst();
            notFull.signal();
            return element;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Returns the current number of elements in the queue.
     *
     * @return the number of elements
     */
    public int size() {
        lock.lock();
        try {
            return elements.size();
        } finally {
            lock.unlock();
        }
    }

    /**
     * Returns the number of additional elements the queue can accept.
     *
     * @return the remaining capacity
     */
    public int remainingCapacity() {
        lock.lock();
        try {
            return capacity - elements.size();
        } finally {
            lock.unlock();
        }
    }

    /**
     * Returns whether the queue contains no elements.
     *
     * @return true if the queue is empty
     */
    public boolean isEmpty() {
        lock.lock();
        try {
            return elements.isEmpty();
        } finally {
            lock.unlock();
        }
    }

    /**
     * Returns the maximum number of elements this queue can hold.
     *
     * @return the queue capacity
     */
    public int capacity() {
        return capacity;
    }
}
