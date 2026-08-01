// Project Euler Problem 44: pentagonal numbers with pentagonal sum and difference.
public class Problem044 {
    public static void main(String[] args) {
        long best = Long.MAX_VALUE;
        // This bound contains the first (and minimal) qualifying pair.
        for (int k = 2; k <= 5_000; k++) {
            long pk = pentagonal(k);
            for (int j = k - 1; j >= 1; j--) {
                long pj = pentagonal(j);
                long difference = pk - pj;
                if (difference >= best) break;
                if (isPentagonal(difference) && isPentagonal(pk + pj)) best = difference;
            }
        }
        System.out.println(best);
    }

    private static long pentagonal(long n) {
        return n * (3 * n - 1) / 2;
    }

    private static boolean isPentagonal(long value) {
        long discriminant = 24 * value + 1;
        long root = (long) Math.sqrt(discriminant);
        return root * root == discriminant && (root + 1) % 6 == 0;
    }
}
