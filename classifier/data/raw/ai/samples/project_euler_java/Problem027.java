// Project Euler 27: product a*b for the quadratic producing the most consecutive primes.
public class Problem027 {
    private static boolean isPrime(long n) {
        if (n < 2) return false;
        if (n % 2 == 0) return n == 2;
        for (long d = 3; d * d <= n; d += 2)
            if (n % d == 0) return false;
        return true;
    }

    public static void main(String[] args) {
        int bestRun = 0;
        int bestProduct = 0;
        for (int a = -999; a <= 999; a++) {
            for (int b = -1000; b <= 1000; b++) {
                int n = 0;
                while (isPrime((long) n * n + (long) a * n + b)) n++;
                if (n > bestRun) {
                    bestRun = n;
                    bestProduct = a * b;
                }
            }
        }
        System.out.println(bestProduct);
    }
}
