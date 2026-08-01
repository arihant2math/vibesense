/** Project Euler 37: sum of the eleven primes truncatable from left and right. */
public class Problem037 {
    public static void main(String[] args) {
        boolean[] prime = sieve(1_000_000);
        int count = 0;
        int sum = 0;
        for (int n = 11; count < 11; n += 2) {
            if (prime[n] && isTruncatable(n, prime)) {
                sum += n;
                count++;
            }
        }
        System.out.println(sum);
    }

    private static boolean isTruncatable(int n, boolean[] prime) {
        for (int right = n / 10; right > 0; right /= 10) {
            if (!prime[right]) {
                return false;
            }
        }
        int divisor = 1;
        while (divisor <= n / 10) {
            divisor *= 10;
        }
        for (int left = n; divisor > 1; divisor /= 10) {
            left %= divisor;
            if (!prime[left]) {
                return false;
            }
        }
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
