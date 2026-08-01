// Project Euler Problem 45: next triangular, pentagonal, and hexagonal number.
public class Problem045 {
    public static void main(String[] args) {
        for (long n = 144; ; n++) {
            long hexagonal = n * (2 * n - 1);
            if (isPentagonal(hexagonal)) {
                System.out.println(hexagonal);
                return;
            }
        }
    }

    private static boolean isPentagonal(long value) {
        long discriminant = 24 * value + 1;
        long root = (long) Math.sqrt(discriminant);
        return root * root == discriminant && (root + 1) % 6 == 0;
    }
}
