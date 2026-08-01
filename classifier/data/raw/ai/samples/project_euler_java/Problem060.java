// Project Euler 60: lowest-sum set of five primes that concatenate pairwise to primes.
import java.util.ArrayList;
import java.util.List;

public class Problem060 {
    private static final int LIMIT = 10_000;
    private static boolean[][] compatible;
    private static int best = Integer.MAX_VALUE;
    private static List<Integer> primes;
    private static int[] trialPrimes;

    public static void main(String[] args) {
        primes = sieve(LIMIT);
        primes.remove(Integer.valueOf(5)); // Any larger prime followed by 5 is divisible by 5.
        trialPrimes = sieve((int) Math.sqrt((long) LIMIT * LIMIT * 10)).stream()
                           .mapToInt(Integer::intValue).toArray();
        compatible = new boolean[primes.size()][primes.size()];
        for (int i = 0; i < primes.size(); i++) {
            for (int j = i + 1; j < primes.size(); j++) {
                int first = primes.get(i), second = primes.get(j);
                compatible[i][j] = compatible[j][i] =
                        isPrime(concatenate(first, second)) && isPrime(concatenate(second, first));
            }
        }
        int[] candidates = new int[primes.size()];
        for (int i = 0; i < candidates.length; i++) candidates[i] = i;
        search(0, 0, candidates);
        System.out.println(best);
    }

    private static void search(int chosen, int sum, int[] candidates) {
        int needed = 5 - chosen;
        if (candidates.length < needed) return;
        int lowerBound = sum;
        for (int i = 0; i < needed; i++) lowerBound += primes.get(candidates[i]);
        if (lowerBound >= best) return;
        if (chosen == 5) {
            best = Math.min(best, sum);
            return;
        }
        for (int i = 0; i <= candidates.length - needed; i++) {
            int selected = candidates[i];
            int[] next = new int[candidates.length - i - 1];
            int count = 0;
            for (int j = i + 1; j < candidates.length; j++) {
                if (compatible[selected][candidates[j]]) next[count++] = candidates[j];
            }
            int[] trimmed = new int[count];
            System.arraycopy(next, 0, trimmed, 0, count);
            search(chosen + 1, sum + primes.get(selected), trimmed);
        }
    }

    private static long concatenate(int left, int right) {
        long factor = 10;
        while (factor <= right) factor *= 10;
        return left * factor + right;
    }

    private static boolean isPrime(long n) {
        if (n < 2) return false;
        for (int p : trialPrimes) {
            if ((long) p * p > n) return true;
            if (n % p == 0) return n == p;
        }
        return true;
    }

    private static List<Integer> sieve(int limit) {
        boolean[] composite = new boolean[limit + 1];
        List<Integer> result = new ArrayList<>();
        for (int n = 2; n <= limit; n++) {
            if (!composite[n]) {
                result.add(n);
                if ((long) n * n <= limit) {
                    for (int multiple = n * n; multiple <= limit; multiple += n) composite[multiple] = true;
                }
            }
        }
        return result;
    }
}
