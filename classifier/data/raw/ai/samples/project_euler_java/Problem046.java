// Project Euler Problem 46: smallest odd composite not satisfying Goldbach's other conjecture.
public class Problem046 {
    public static void main(String[] args) {
        for (int n = 9; ; n += 2) {
            if (!isPrime(n) && !isSumOfPrimeAndTwiceSquare(n)) {
                System.out.println(n);
                return;
            }
        }
    }

    private static boolean isSumOfPrimeAndTwiceSquare(int n) {
        for (int square = 1; 2 * square * square < n; square++) {
            if (isPrime(n - 2 * square * square)) return true;
        }
        return false;
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++) if (n % d == 0) return false;
        return true;
    }
}
