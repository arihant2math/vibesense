// Project Euler 71: numerator immediately left of 3/7 for denominators up to one million.
public class Problem071 {
    public static void main(String[] args) {
        int bestNumerator = 0;
        int bestDenominator = 1;
        for (int d = 2; d <= 1_000_000; d++) {
            int n = (3 * d - 1) / 7;
            if (gcd(n, d) == 1 && (long) n * bestDenominator > (long) bestNumerator * d) {
                bestNumerator = n;
                bestDenominator = d;
            }
        }
        System.out.println(bestNumerator);
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
