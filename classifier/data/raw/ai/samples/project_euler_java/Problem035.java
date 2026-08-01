/** Project Euler 35: count circular primes below one million. */
public class Problem035 {
    private static final int LIMIT = 1_000_000;

    public static void main(String[] args) {
        boolean[] prime = sieve(LIMIT);
        int count = 0;
        for (int n = 2; n < LIMIT; n++) {
            if (prime[n] && isCircularPrime(n, prime)) {
                count++;
            }
        }
        System.out.println(count);
    }

    private static boolean isCircularPrime(int n, boolean[] prime) {
        int divisor = 1;
        for (int value = n; value >= 10; value /= 10) {
            divisor *= 10;
        }
        int rotation = n;
        do {
            if (!prime[rotation]) {
                return false;
            }
            rotation = (rotation % divisor) * 10 + rotation / divisor;
        } while (rotation != n);
        return true;
    }

    private static boolean[] sieve(int limit) {
        boolean[] prime = new boolean[limit];
        for (int i = 2; i < limit; i++) {
            prime[i] = true;
        }
        for (int i = 2; i * i < limit; i++) {
            if (prime[i]) {
                for (int multiple = i * i; multiple < limit; multiple += i) {
                    prime[multiple] = false;
                }
            }
        }
        return prime;
    }
}
