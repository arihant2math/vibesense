// Project Euler 58: spiral side length when diagonal-prime ratio first falls below ten percent.
public class Problem058 {
    public static void main(String[] args) {
        int primes = 0;
        int diagonalCount = 1;
        for (int side = 3; ; side += 2) {
            int square = side * side;
            int step = side - 1;
            for (int corner = 1; corner <= 3; corner++) {
                if (isPrime(square - corner * step)) primes++;
            }
            diagonalCount += 4;
            if (10 * primes < diagonalCount) {
                System.out.println(side);
                return;
            }
        }
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n % 2 == 0) return n == 2;
        for (int d = 3; d <= n / d; d += 2) {
            if (n % d == 0) return false;
        }
        return true;
    }
}
