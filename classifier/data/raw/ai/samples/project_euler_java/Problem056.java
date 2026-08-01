// Project Euler 56: greatest digital sum of a^b for a, b < 100.
import java.math.BigInteger;

public class Problem056 {
    public static void main(String[] args) {
        int best = 0;
        for (int a = 1; a < 100; a++) {
            BigInteger base = BigInteger.valueOf(a);
            for (int b = 1; b < 100; b++) {
                int sum = 0;
                for (char digit : base.pow(b).toString().toCharArray()) sum += digit - '0';
                best = Math.max(best, sum);
            }
        }
        System.out.println(best);
    }
}
