import java.time.Duration;
import java.time.Instant;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.Objects;
import java.util.function.Supplier;

public final class SlidingWindowRateLimiter {
    private final int maxRequests;
    private final long windowNanos;
    private final Supplier<Long> nanoTimeSupplier;
    private final Deque<Long> requestTimes = new ArrayDeque<>();

    public SlidingWindowRateLimiter(int maxRequests, Duration window) {
        this(maxRequests, window, System::nanoTime);
    }

    public SlidingWindowRateLimiter(
            int maxRequests,
            Duration window,
            Supplier<Long> nanoTimeSupplier) {
        if (maxRequests <= 0) {
            throw new IllegalArgumentException("maxRequests must be positive");
        }
        Objects.requireNonNull(window, "window");
        Objects.requireNonNull(nanoTimeSupplier, "nanoTimeSupplier");

        long nanos = window.toNanos();
        if (nanos <= 0) {
            throw new IllegalArgumentException("window must be positive");
        }

        this.maxRequests = maxRequests;
        this.windowNanos = nanos;
        this.nanoTimeSupplier = nanoTimeSupplier;
    }

    public synchronized boolean tryAcquire() {
        long now = nanoTimeSupplier.get();
        evictExpired(now);

        if (requestTimes.size() >= maxRequests) {
            return false;
        }

        requestTimes.addLast(now);
        return true;
    }

    public synchronized int availablePermits() {
        long now = nanoTimeSupplier.get();
        evictExpired(now);
        return maxRequests - requestTimes.size();
    }

    public synchronized int requestCount() {
        long now = nanoTimeSupplier.get();
        evictExpired(now);
        return requestTimes.size();
    }

    public synchronized void reset() {
        requestTimes.clear();
    }

    private void evictExpired(long now) {
        long windowStart = now - windowNanos;
        while (!requestTimes.isEmpty() && requestTimes.peekFirst() <= windowStart) {
            requestTimes.removeFirst();
        }
    }
}
