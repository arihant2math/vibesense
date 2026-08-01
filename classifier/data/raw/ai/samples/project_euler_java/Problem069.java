// Project Euler 69: n <= one million maximizing n / phi(n).

public class Problem069 {
    public static void main(String[] args) {
        int limit = 1_000_000;
        int product = 1;
        for (int candidate = 2; ; candidate++) {
            if (!prime(candidate)) continue;
            if (product > limit / candidate) break;
            product *= candidate;
        }
        System.out.println(product);
    }

    static boolean prime(int n) {
        for (int d = 2; d * d <= n; d++) if (n % d == 0) return false;
        return true;
    }
}
