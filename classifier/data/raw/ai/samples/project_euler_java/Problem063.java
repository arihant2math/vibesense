// Project Euler 63: n-digit positive integers that are also nth powers.
import java.math.BigInteger;

public class Problem063 {
    public static void main(String[] args) {
        int count = 0;
        for (int exponent = 1;; exponent++) {
            boolean found = false;
            for (int base = 1; base <= 9; base++) {
                int digits = BigInteger.valueOf(base).pow(exponent).toString().length();
                if (digits == exponent) { count++; found = true; }
            }
            if (!found) break;
        }
        System.out.println(count);
    }
}
