import java.util.HashSet;
import java.util.Set;

/** Project Euler 087: prime square, cube, and fourth-power sums. */
public class Problem087 {
    public static void main(String[] args) {
        final int limit = 50_000_000;
        int[] primes = primesBelow((int) Math.sqrt(limit));
        Set<Integer> values = new HashSet<>();
        for (int p : primes) {
            long fourth = (long) p * p * p * p;
            if (fourth >= limit) break;
            for (int q : primes) {
                long cube = (long) q * q * q;
                if (fourth + cube >= limit) break;
                for (int r : primes) {
                    long sum = fourth + cube + (long) r * r;
                    if (sum >= limit) break;
                    values.add((int) sum);
                }
            }
        }
        System.out.println(values.size());
    }

    private static int[] primesBelow(int limit) {
        boolean[] composite = new boolean[limit + 1];
        int count = 0;
        for (int n = 2; n <= limit; n++) {
            if (!composite[n]) {
                count++;
                if ((long) n * n <= limit) for (int m = n * n; m <= limit; m += n) composite[m] = true;
            }
        }
        int[] primes = new int[count]; int i = 0;
        for (int n = 2; n <= limit; n++) if (!composite[n]) primes[i++] = n;
        return primes;
    }
}
