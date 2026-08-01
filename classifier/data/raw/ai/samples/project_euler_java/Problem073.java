// Project Euler 73: count reduced fractions strictly between 1/3 and 1/2.
public class Problem073 {
    public static void main(String[] args) {
        final int limit = 12_000;
        int count = 0;
        for (int denominator = 2; denominator <= limit; denominator++) {
            int first = denominator / 3 + 1;
            int last = (denominator - 1) / 2;
            for (int numerator = first; numerator <= last; numerator++) {
                if (gcd(numerator, denominator) == 1) {
                    count++;
                }
            }
        }
        System.out.println(count);
    }

    private static int gcd(int a, int b) {
        while (b != 0) {
            int t = a % b;
            a = b;
            b = t;
        }
        return a;
    }
}
