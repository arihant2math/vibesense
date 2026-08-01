// Project Euler 21: sum of amicable numbers below 10,000.
public class Problem021 {
    private static int properDivisorSum(int n) {
        if (n < 2) return 0;
        int sum = 1;
        for (int d = 2; d * d <= n; d++) {
            if (n % d == 0) {
                sum += d;
                if (d * d != n) sum += n / d;
            }
        }
        return sum;
    }

    public static void main(String[] args) {
        int total = 0;
        for (int a = 2; a < 10_000; a++) {
            int b = properDivisorSum(a);
            if (b != a && properDivisorSum(b) == a) total += a;
        }
        System.out.println(total);
    }
}
