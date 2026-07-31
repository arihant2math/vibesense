import java.time.Instant;
import java.util.Objects;

public final class RingBufferLogger {
    public enum Severity {
        DEBUG,
        INFO,
        WARN,
        ERROR
    }

    public static final class LogEntry {
        private final Instant timestamp;
        private final Severity severity;
        private final String message;

        private LogEntry(Instant timestamp, Severity severity, String message) {
            this.timestamp = timestamp;
            this.severity = severity;
            this.message = message;
        }

        public Instant getTimestamp() {
            return timestamp;
        }

        public Severity getSeverity() {
            return severity;
        }

        public String getMessage() {
            return message;
        }

        @Override
        public String toString() {
            return timestamp + " [" + severity + "] " + message;
        }
    }

    private final LogEntry[] entries;
    private Severity minimumSeverity;
    private int nextIndex;
    private int size;

    public RingBufferLogger(int capacity) {
        this(capacity, Severity.DEBUG);
    }

    public RingBufferLogger(int capacity, Severity minimumSeverity) {
        if (capacity <= 0) {
            throw new IllegalArgumentException("Capacity must be greater than zero");
        }

        this.entries = new LogEntry[capacity];
        this.minimumSeverity = Objects.requireNonNull(minimumSeverity);
    }

    public synchronized void setMinimumSeverity(Severity minimumSeverity) {
        this.minimumSeverity = Objects.requireNonNull(minimumSeverity);
    }

    public synchronized Severity getMinimumSeverity() {
        return minimumSeverity;
    }

    public void debug(String message) {
        log(Severity.DEBUG, message);
    }

    public void info(String message) {
        log(Severity.INFO, message);
    }

    public void warn(String message) {
        log(Severity.WARN, message);
    }

    public void error(String message) {
        log(Severity.ERROR, message);
    }

    public synchronized void log(Severity severity, String message) {
        Objects.requireNonNull(severity);
        Objects.requireNonNull(message);

        if (severity.ordinal() < minimumSeverity.ordinal()) {
            return;
        }

        entries[nextIndex] = new LogEntry(Instant.now(), severity, message);
        nextIndex = (nextIndex + 1) % entries.length;

        if (size < entries.length) {
            size++;
        }
    }

    public synchronized LogEntry[] snapshot() {
        LogEntry[] result = new LogEntry[size];
        int oldestIndex = (nextIndex - size + entries.length) % entries.length;

        for (int i = 0; i < size; i++) {
            result[i] = entries[(oldestIndex + i) % entries.length];
        }

        return result;
    }

    public synchronized int size() {
        return size;
    }

    public int capacity() {
        return entries.length;
    }

    public synchronized void clear() {
        for (int i = 0; i < entries.length; i++) {
            entries[i] = null;
        }

        nextIndex = 0;
        size = 0;
    }
}
