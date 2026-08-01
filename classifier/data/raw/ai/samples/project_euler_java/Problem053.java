// Project Euler 53: count binomial coefficients over one million for n <= 100.
import java.math.BigInteger;

public class Problem053 {
    public static void main(String[] args) {
        BigInteger limit = BigInteger.valueOf(1_000_000);
        int count = 0;
        for (int n = 1; n <= 100; n++) {
            BigInteger coefficient = BigInteger.ONE;
            for (int r = 1; r <= n; r++) {
                coefficient = coefficient.multiply(BigInteger.valueOf(n - r + 1))
                                         .divide(BigInteger.valueOf(r));
                if (coefficient.compareTo(limit) > 0) count++;
            }
        }
        System.out.println(count);
    }
}
