// Project Euler Problem 50: prime below one million expressible as the longest prime sum.
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Problem050 {
    private static final int LIMIT = 1_000_000;

    public static void main(String[] args) {
        boolean[] prime = sieve(LIMIT);
        List<Integer> primes = new ArrayList<Integer>();
        for (int n = 2; n < LIMIT; n++) if (prime[n]) primes.add(n);

        long[] prefix = new long[primes.size() + 1];
        for (int i = 0; i < primes.size(); i++) prefix[i + 1] = prefix[i] + primes.get(i);
        int maxLength = 0;
        while (maxLength + 1 < prefix.length && prefix[maxLength + 1] < LIMIT) maxLength++;

        for (int length = maxLength; length > 0; length--) {
            for (int start = 0; start + length <= primes.size(); start++) {
                long sum = prefix[start + length] - prefix[start];
                if (sum >= LIMIT) break;
                if (prime[(int) sum]) {
                    System.out.println(sum);
                    return;
                }
            }
        }
    }

    private static boolean[] sieve(int limit) {
        boolean[] prime = new boolean[limit];
        Arrays.fill(prime, true);
        prime[0] = prime[1] = false;
        for (int p = 2; p * p < limit; p++) {
            if (prime[p]) for (int multiple = p * p; multiple < limit; multiple += p) prime[multiple] = false;
        }
        return prime;
    }
}
