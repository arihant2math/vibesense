public final class Vector3 {
    private final double x;
    private final double y;
    private final double z;

    public Vector3(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public double x() {
        return x;
    }

    public double y() {
        return y;
    }

    public double z() {
        return z;
    }

    public double dot(Vector3 other) {
        requireNonNull(other);
        return x * other.x + y * other.y + z * other.z;
    }

    public Vector3 cross(Vector3 other) {
        requireNonNull(other);
        return new Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    private static void requireNonNull(Vector3 vector) {
        if (vector == null) {
            throw new NullPointerException("vector must not be null");
        }
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }
        if (!(object instanceof Vector3)) {
            return false;
        }
        Vector3 other = (Vector3) object;
        return Double.compare(x, other.x) == 0
            && Double.compare(y, other.y) == 0
            && Double.compare(z, other.z) == 0;
    }

    @Override
    public int hashCode() {
        int result = Double.hashCode(x);
        result = 31 * result + Double.hashCode(y);
        result = 31 * result + Double.hashCode(z);
        return result;
    }

    @Override
    public String toString() {
        return "Vector3{x=" + x + ", y=" + y + ", z=" + z + "}";
    }
}
