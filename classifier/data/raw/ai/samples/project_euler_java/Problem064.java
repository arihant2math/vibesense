// Project Euler 64: odd periods in continued fractions of square roots.

public class Problem064 {
    public static void main(String[] args) {
        int count = 0;
        for (int d = 2; d <= 10_000; d++) {
            int root = (int) Math.sqrt(d);
            if (root * root == d) continue;
            int m = 0, denominator = 1, a = root, period = 0;
            do {
                m = denominator * a - m;
                denominator = (d - m * m) / denominator;
                a = (root + m) / denominator;
                period++;
            } while (a != 2 * root);
            if ((period & 1) == 1) count++;
        }
        System.out.println(count);
    }
}
