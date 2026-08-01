// Project Euler Problem 41: largest pandigital prime.
public class Problem041 {
    public static void main(String[] args) {
        search(0, 0, 0);
    }

    private static boolean search(int position, int value, int used) {
        if (position == 7) {
            if (isPrime(value)) {
                System.out.println(value);
                return true;
            }
            return false;
        }
        for (int digit = 7; digit >= 1; digit--) {
            if ((used & (1 << digit)) == 0
                    && search(position + 1, value * 10 + digit, used | (1 << digit))) {
                return true;
            }
        }
        return false;
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++) if (n % d == 0) return false;
        return true;
    }
}
