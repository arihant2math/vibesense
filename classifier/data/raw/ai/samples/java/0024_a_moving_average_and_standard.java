import java.util.ArrayDeque;
import java.util.Deque;

/**
 * Accumulates a fixed-size moving window of numeric values and calculates
 * its average and population standard deviation.
 */
public final class MovingStatisticsAccumulator {
    private final int windowSize;
    private final Deque<Double> values = new ArrayDeque<>();
    private double sum;
    private double sumOfSquares;

    /**
     * Creates an accumulator with the specified moving-window size.
     *
     * @param windowSize maximum number of values retained
     * @throws IllegalArgumentException if {@code windowSize} is less than one
     */
    public MovingStatisticsAccumulator(int windowSize) {
        if (windowSize < 1) {
            throw new IllegalArgumentException("windowSize must be at least 1");
        }
        this.windowSize = windowSize;
    }

    /**
     * Adds a value to the moving window.
     *
     * @param value value to add
     * @throws IllegalArgumentException if {@code value} is not finite
     */
    public void add(double value) {
        if (!Double.isFinite(value)) {
            throw new IllegalArgumentException("value must be finite");
        }

        values.addLast(value);
        sum += value;
        sumOfSquares += value * value;

        if (values.size() > windowSize) {
            double removed = values.removeFirst();
            sum -= removed;
            sumOfSquares -= removed * removed;
        }
    }

    /**
     * Returns the moving average of the retained values.
     *
     * @return the average, or {@code 0.0} if no values have been added
     */
    public double getAverage() {
        return values.isEmpty() ? 0.0 : sum / values.size();
    }

    /**
     * Returns the population standard deviation of the retained values.
     *
     * @return the standard deviation, or {@code 0.0} if fewer than two values
     *         are retained
     */
    public double getStandardDeviation() {
        if (values.size() < 2) {
            return 0.0;
        }

        double mean = getAverage();
        double variance = (sumOfSquares / values.size()) - (mean * mean);
        return Math.sqrt(Math.max(0.0, variance));
    }

    /**
     * Returns the number of values currently retained.
     *
     * @return the retained value count
     */
    public int getCount() {
        return values.size();
    }

    /**
     * Returns the configured maximum window size.
     *
     * @return the window size
     */
    public int getWindowSize() {
        return windowSize;
    }

    /**
     * Removes all retained values and resets the accumulator.
     */
    public void clear() {
        values.clear();
        sum = 0.0;
        sumOfSquares = 0.0;
    }
}
