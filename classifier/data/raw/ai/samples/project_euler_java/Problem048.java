// Project Euler Problem 48: last ten digits of the self powers series.
public class Problem048 {
    private static final long MODULUS = 10_000_000_000L;

    public static void main(String[] args) {
        long sum = 0;
        for (int n = 1; n <= 1_000; n++) sum = (sum + powerMod(n, n)) % MODULUS;
        System.out.printf("%010d%n", sum);
    }

    private static long powerMod(long base, int exponent) {
        long result = 1;
        while (exponent > 0) {
            if ((exponent & 1) != 0) result = multiplyMod(result, base);
            base = multiplyMod(base, base);
            exponent >>= 1;
        }
        return result;
    }

    // Addition-and-doubling keeps intermediate products below Long.MAX_VALUE.
    private static long multiplyMod(long left, long right) {
        long result = 0;
        while (right > 0) {
            if ((right & 1) != 0) result = (result + left) % MODULUS;
            left = (left + left) % MODULUS;
            right >>= 1;
        }
        return result;
    }
}
