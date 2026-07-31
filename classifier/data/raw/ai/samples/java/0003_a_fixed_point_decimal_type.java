import java.math.BigInteger;
import java.util.Objects;

public final class FixedPointDecimal {
    private final BigInteger unscaledValue;
    private final int scale;

    public FixedPointDecimal(long value) {
        this(BigInteger.valueOf(value), 0);
    }

    public FixedPointDecimal(String value) {
        Objects.requireNonNull(value, "value");

        String input = value.trim();
        if (input.isEmpty()) {
            throw new NumberFormatException("Empty decimal value");
        }

        boolean negative = input.charAt(0) == '-';
        if (negative || input.charAt(0) == '+') {
            input = input.substring(1);
        }

        String[] parts = input.split("\\.", -1);
        if (parts.length > 2 || input.isEmpty()) {
            throw new NumberFormatException("Invalid decimal value: " + value);
        }

        String whole = parts[0].isEmpty() ? "0" : parts[0];
        String fraction = parts.length == 2 ? parts[1] : "";

        if (!whole.matches("\\d+") || !fraction.matches("\\d*")) {
            throw new NumberFormatException("Invalid decimal value: " + value);
        }

        String digits = whole + fraction;
        BigInteger parsed = new BigInteger(digits.isEmpty() ? "0" : digits);

        this.unscaledValue = negative ? parsed.negate() : parsed;
        this.scale = fraction.length();
    }

    private FixedPointDecimal(BigInteger unscaledValue, int scale) {
        if (scale < 0) {
            throw new IllegalArgumentException("Scale cannot be negative");
        }
        this.unscaledValue = unscaledValue;
        this.scale = scale;
    }

    public FixedPointDecimal add(FixedPointDecimal other) {
        Objects.requireNonNull(other, "other");

        int resultScale = Math.max(scale, other.scale);
        BigInteger left = unscaledValue.multiply(powerOfTen(resultScale - scale));
        BigInteger right = other.unscaledValue.multiply(powerOfTen(resultScale - other.scale));

        return new FixedPointDecimal(left.add(right), resultScale);
    }

    public FixedPointDecimal multiply(FixedPointDecimal other) {
        Objects.requireNonNull(other, "other");

        return new FixedPointDecimal(
                unscaledValue.multiply(other.unscaledValue),
                scale + other.scale
        );
    }

    public static FixedPointDecimal sum(FixedPointDecimal... values) {
        if (values == null || values.length == 0) {
            return new FixedPointDecimal(0);
        }

        FixedPointDecimal result = new FixedPointDecimal(0);
        for (FixedPointDecimal value : values) {
            result = result.add(Objects.requireNonNull(value, "value"));
        }
        return result;
    }

    public static FixedPointDecimal product(FixedPointDecimal... values) {
        if (values == null || values.length == 0) {
            return new FixedPointDecimal(1);
        }

        FixedPointDecimal result = new FixedPointDecimal(1);
        for (FixedPointDecimal value : values) {
            result = result.multiply(Objects.requireNonNull(value, "value"));
        }
        return result;
    }

    public BigInteger unscaledValue() {
        return unscaledValue;
    }

    public int scale() {
        return scale;
    }

    private static BigInteger powerOfTen(int exponent) {
        return BigInteger.TEN.pow(exponent);
    }

    @Override
    public String toString() {
        if (scale == 0) {
            return unscaledValue.toString();
        }

        boolean negative = unscaledValue.signum() < 0;
        String digits = unscaledValue.abs().toString();

        if (digits.length() <= scale) {
            digits = "0".repeat(scale + 1 - digits.length()) + digits;
        }

        int split = digits.length() - scale;
        String result = digits.substring(0, split) + "." + digits.substring(split);
        return negative ? "-" + result : result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof FixedPointDecimal)) {
            return false;
        }

        FixedPointDecimal other = (FixedPointDecimal) obj;
        int commonScale = Math.max(scale, other.scale);

        return unscaledValue.multiply(powerOfTen(commonScale - scale))
                .equals(other.unscaledValue.multiply(powerOfTen(commonScale - other.scale)));
    }

    @Override
    public int hashCode() {
        String normalized = toString();
        return normalized.hashCode();
    }
}
