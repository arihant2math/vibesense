// Project Euler 77: first integer expressible as a sum of primes in over 5000 ways.
import java.util.ArrayList;
import java.util.List;

public class Problem077 {
    public static void main(String[] args) {
        for (int n = 2; ; n++) {
            long[] ways = new long[n + 1];
            ways[0] = 1;
            for (int prime : primesUpTo(n)) {
                for (int sum = prime; sum <= n; sum++) {
                    ways[sum] += ways[sum - prime];
                }
            }
            if (ways[n] > 5000) {
                System.out.println(n);
                return;
            }
        }
    }

    private static List<Integer> primesUpTo(int limit) {
        boolean[] composite = new boolean[limit + 1];
        List<Integer> primes = new ArrayList<>();
        for (int n = 2; n <= limit; n++) {
            if (!composite[n]) {
                primes.add(n);
                if ((long) n * n <= limit) {
                    for (int multiple = n * n; multiple <= limit; multiple += n) {
                        composite[multiple] = true;
                    }
                }
            }
        }
        return primes;
    }
}
