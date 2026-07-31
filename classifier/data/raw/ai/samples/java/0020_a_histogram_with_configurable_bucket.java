import java.util.Arrays;

public final class Histogram {
    private final double[] boundaries;
    private final long[] counts;

    public Histogram(double... boundaries) {
        if (boundaries == null || boundaries.length == 0) {
            throw new IllegalArgumentException("At least one bucket boundary is required");
        }

        this.boundaries = boundaries.clone();

        for (int i = 0; i < this.boundaries.length; i++) {
            if (Double.isNaN(this.boundaries[i])) {
                throw new IllegalArgumentException("Bucket boundaries cannot contain NaN");
            }
            if (i > 0 && this.boundaries[i] <= this.boundaries[i - 1]) {
                throw new IllegalArgumentException("Bucket boundaries must be strictly increasing");
            }
        }

        this.counts = new long[this.boundaries.length + 1];
    }

    public void record(double value) {
        if (Double.isNaN(value)) {
            throw new IllegalArgumentException("Value cannot be NaN");
        }

        int bucket = Arrays.binarySearch(boundaries, value);
        if (bucket < 0) {
            bucket = -bucket - 1;
        } else {
            while (bucket + 1 < boundaries.length && boundaries[bucket + 1] == value) {
                bucket++;
            }
        }

        counts[bucket]++;
    }

    public long[] getCounts() {
        return counts.clone();
    }

    public double[] getBoundaries() {
        return boundaries.clone();
    }

    public long getCount(double value) {
        if (Double.isNaN(value)) {
            throw new IllegalArgumentException("Value cannot be NaN");
        }

        int bucket = Arrays.binarySearch(boundaries, value);
        if (bucket < 0) {
            bucket = -bucket - 1;
        }

        return counts[bucket];
    }

    public long totalCount() {
        long total = 0;
        for (long count : counts) {
            total += count;
        }
        return total;
    }

    @Override
    public String toString() {
        return "Histogram{boundaries=" + Arrays.toString(boundaries)
                + ", counts=" + Arrays.toString(counts) + '}';
    }
}
