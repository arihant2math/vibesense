import java.time.Instant;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.time.temporal.TemporalAccessor;

public final class Iso8601DateParser {
    private static final DateTimeFormatter ISO_FORMATTER = DateTimeFormatter.ISO_OFFSET_DATE_TIME;

    private Iso8601DateParser() {
    }

    public static OffsetDateTime parse(String timestamp) {
        if (timestamp == null || timestamp.isBlank()) {
            throw new IllegalArgumentException("Timestamp must not be null or blank");
        }

        try {
            TemporalAccessor parsed = DateTimeFormatter.ISO_DATE_TIME.parse(timestamp);
            return OffsetDateTime.from(parsed);
        } catch (DateTimeParseException | RuntimeException exception) {
            throw new IllegalArgumentException("Invalid ISO 8601 timestamp: " + timestamp, exception);
        }
    }

    public static String format(OffsetDateTime dateTime) {
        if (dateTime == null) {
            throw new IllegalArgumentException("Date-time must not be null");
        }

        return ISO_FORMATTER.format(dateTime);
    }

    public static String format(Instant instant) {
        if (instant == null) {
            throw new IllegalArgumentException("Instant must not be null");
        }

        return ISO_FORMATTER.format(instant.atOffset(ZoneOffset.UTC));
    }

    public static void main(String[] args) {
        String input = "2024-01-15T10:30:00+05:30";
        OffsetDateTime parsed = parse(input);
        System.out.println(format(parsed));
        System.out.println(format(parsed.toInstant()));
    }
}
