import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

public final class InMemoryPageCursor<T> {
    private final List<T> items;
    private final int pageSize;
    private int offset;

    public InMemoryPageCursor(Iterable<T> source, int pageSize) {
        Objects.requireNonNull(source, "source must not be null");
        if (pageSize <= 0) {
            throw new IllegalArgumentException("pageSize must be greater than zero");
        }

        List<T> snapshot = new ArrayList<>();
        for (T item : source) {
            snapshot.add(item);
        }

        this.items = Collections.unmodifiableList(snapshot);
        this.pageSize = pageSize;
        this.offset = 0;
    }

    public boolean hasNext() {
        return offset < items.size();
    }

    public Optional<Page<T>> nextPage() {
        if (!hasNext()) {
            return Optional.empty();
        }

        int start = offset;
        int end = Math.min(start + pageSize, items.size());
        offset = end;

        return Optional.of(new Page<>(
                items.subList(start, end),
                start / pageSize,
                end < items.size()
        ));
    }

    public void reset() {
        offset = 0;
    }

    public int pageSize() {
        return pageSize;
    }

    public int position() {
        return offset;
    }

    public int totalSize() {
        return items.size();
    }

    public static final class Page<T> {
        private final List<T> items;
        private final int pageNumber;
        private final boolean hasNext;

        private Page(List<T> items, int pageNumber, boolean hasNext) {
            this.items = Collections.unmodifiableList(new ArrayList<>(items));
            this.pageNumber = pageNumber;
            this.hasNext = hasNext;
        }

        public List<T> items() {
            return items;
        }

        public int pageNumber() {
            return pageNumber;
        }

        public boolean hasNext() {
            return hasNext;
        }
    }
}
