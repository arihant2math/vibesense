// Project Euler 80: total of the first one hundred decimal digits of irrational square roots.
import java.math.BigInteger;

public class Problem080 {
    public static void main(String[] args) {
        BigInteger scale = BigInteger.TEN.pow(198);
        int total = 0;
        for (int n = 1; n <= 100; n++) {
            int root = (int) Math.sqrt(n);
            if (root * root == n) {
                continue;
            }
            String digits = sqrtFloor(BigInteger.valueOf(n).multiply(scale)).toString();
            for (int i = 0; i < digits.length(); i++) {
                total += digits.charAt(i) - '0';
            }
        }
        System.out.println(total);
    }

    private static BigInteger sqrtFloor(BigInteger value) {
        BigInteger estimate = BigInteger.ONE.shiftLeft((value.bitLength() + 1) / 2);
        while (true) {
            BigInteger next = estimate.add(value.divide(estimate)).shiftRight(1);
            if (next.compareTo(estimate) >= 0) {
                return estimate;
            }
            estimate = next;
        }
    }
}
