import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Random;
import java.util.stream.Stream;

public final class ReservoirSampler {
    private ReservoirSampler() {
        throw new AssertionError("Utility class must not be instantiated");
    }

    public static <T> List<T> sample(
            Stream<? extends T> stream,
            int sampleSize,
            Random random) {

        Objects.requireNonNull(stream, "stream must not be null");
        Objects.requireNonNull(random, "random must not be null");

        if (sampleSize < 0) {
            throw new IllegalArgumentException(
                    "sampleSize must be non-negative: " + sampleSize);
        }

        if (sampleSize == 0) {
            stream.close();
            return Collections.emptyList();
        }

        List<T> reservoir = new ArrayList<>(sampleSize);
        long itemsSeen = 0;

        try (Stream<? extends T> input = stream.sequential()) {
            var iterator = input.iterator();

            while (iterator.hasNext()) {
                T item = iterator.next();

                if (itemsSeen == Long.MAX_VALUE) {
                    throw new IllegalStateException(
                            "Input stream contains too many elements");
                }

                itemsSeen++;

                if (reservoir.size() < sampleSize) {
                    reservoir.add(item);
                } else {
                    long replacementIndex = random.nextLong(itemsSeen);
                    if (replacementIndex < sampleSize) {
                        reservoir.set((int) replacementIndex, item);
                    }
                }
            }
        }

        return Collections.unmodifiableList(reservoir);
    }
}
