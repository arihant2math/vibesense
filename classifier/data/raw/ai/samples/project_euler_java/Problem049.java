// Project Euler Problem 49: arithmetic sequences of permuted four-digit primes.
import java.util.Arrays;

public class Problem049 {
    public static void main(String[] args) {
        for (int first = 1_000; first < 10_000; first++) {
            if (!isPrime(first)) continue;
            for (int second = first + 1; second < 10_000; second++) {
                int third = second + (second - first);
                if (third >= 10_000) break;
                if (isPrime(second) && isPrime(third) && sameDigits(first, second) && sameDigits(first, third)
                        && first != 1_487) {
                    System.out.println("" + first + second + third);
                    return;
                }
            }
        }
    }

    private static boolean sameDigits(int first, int second) {
        char[] a = String.valueOf(first).toCharArray();
        char[] b = String.valueOf(second).toCharArray();
        Arrays.sort(a);
        Arrays.sort(b);
        return Arrays.equals(a, b);
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++) if (n % d == 0) return false;
        return true;
    }
}
