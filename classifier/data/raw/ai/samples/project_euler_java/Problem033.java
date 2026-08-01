/** Project Euler 33: denominator of the product of non-trivial digit-cancelling fractions. */
public class Problem033 {
    public static void main(String[] args) {
        int numeratorProduct = 1;
        int denominatorProduct = 1;
        for (int numerator = 10; numerator < 100; numerator++) {
            for (int denominator = numerator + 1; denominator < 100; denominator++) {
                int a = numerator / 10;
                int b = numerator % 10;
                int c = denominator / 10;
                int d = denominator % 10;
                if (b != 0 && b == c && numerator * d == denominator * a) {
                    numeratorProduct *= a;
                    denominatorProduct *= d;
                }
            }
        }
        int gcd = gcd(numeratorProduct, denominatorProduct);
        System.out.println(denominatorProduct / gcd);
    }

    private static int gcd(int a, int b) {
        while (b != 0) {
            int remainder = a % b;
            a = b;
            b = remainder;
        }
        return a;
    }
}
