/** Project Euler 32: sum of products whose multiplicand/product identity is 1-to-9 pandigital. */
public class Problem032 {
    public static void main(String[] args) {
        boolean[] seenProducts = new boolean[10000];
        int sum = 0;
        for (int a = 2; a < 100; a++) {
            for (int b = 123; b < 10000 / a; b++) {
                int product = a * b;
                if (isPandigital(a, b, product) && !seenProducts[product]) {
                    seenProducts[product] = true;
                    sum += product;
                }
            }
        }
        System.out.println(sum);
    }

    private static boolean isPandigital(int a, int b, int product) {
        int mask = 0;
        int digits = 0;
        int[] values = {a, b, product};
        for (int value : values) {
            while (value > 0) {
                int digit = value % 10;
                if (digit == 0 || (mask & (1 << digit)) != 0) {
                    return false;
                }
                mask |= 1 << digit;
                digits++;
                value /= 10;
            }
        }
        return digits == 9 && mask == 0x3FE;
    }
}
