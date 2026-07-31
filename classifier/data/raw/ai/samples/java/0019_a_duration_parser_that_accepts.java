import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Parses compact duration strings such as {@code 1h30m}.
 */
public final class DurationParser {
    private static final Pattern TOKEN =
            Pattern.compile("(\\d+)(ms|[dhms])");

    private DurationParser() {
    }

    /**
     * Parses a duration string containing non-negative integer values followed
     * by {@code d}, {@code h}, {@code m}, {@code s}, or {@code ms} units.
     *
     * @param value the duration string to parse
     * @return the parsed duration
     * @throws IllegalArgumentException if the input is null, empty, malformed,
     *                                  or exceeds {@link Duration}'s range
     */
    public static Duration parse(String value) {
        if (value == null || value.isBlank()) {
            throw new IllegalArgumentException("Duration must not be null or blank");
        }

        String input = value.trim();
        Matcher matcher = TOKEN.matcher(input);
        Duration result = Duration.ZERO;
        int position = 0;

        while (matcher.find()) {
            if (matcher.start() != position) {
                throw new IllegalArgumentException("Invalid duration: " + value);
            }

            long amount;
            try {
                amount = Long.parseLong(matcher.group(1));
            } catch (NumberFormatException exception) {
                throw new IllegalArgumentException("Invalid duration: " + value, exception);
            }

            ChronoUnit unit = switch (matcher.group(2)) {
                case "d" -> ChronoUnit.DAYS;
                case "h" -> ChronoUnit.HOURS;
                case "m" -> ChronoUnit.MINUTES;
                case "s" -> ChronoUnit.SECONDS;
                case "ms" -> ChronoUnit.MILLIS;
                default -> throw new IllegalArgumentException("Invalid duration: " + value);
            };

            try {
                result = result.plus(amount, unit);
            } catch (ArithmeticException exception) {
                throw new IllegalArgumentException("Duration exceeds supported range: " + value, exception);
            }

            position = matcher.end();
        }

        if (position != input.length()) {
            throw new IllegalArgumentException("Invalid duration: " + value);
        }

        return result;
    }
}
