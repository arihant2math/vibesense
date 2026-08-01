// Project Euler Problem 12: first triangular number with over 500 divisors.
public class Problem012 {
    public static void main(String[] args) {
        for (long n = 1; ; n++) {
            long triangle = n * (n + 1) / 2;
            if (divisorCount(triangle) > 500) {
                System.out.println(triangle);
                return;
            }
        }
    }

    private static int divisorCount(long number) {
        int count = 1;
        for (long factor = 2; factor * factor <= number; factor += factor == 2 ? 1 : 2) {
            int exponent = 0;
            while (number % factor == 0) {
                number /= factor;
                exponent++;
            }
            count *= exponent + 1;
        }
        return count * (number > 1 ? 2 : 1);
    }
}
