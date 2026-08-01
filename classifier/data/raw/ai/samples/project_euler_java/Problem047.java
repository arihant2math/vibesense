// Project Euler Problem 47: first four consecutive integers with four distinct prime factors.
public class Problem047 {
    public static void main(String[] args) {
        int consecutive = 0;
        for (int n = 2; ; n++) {
            if (distinctPrimeFactorCount(n) == 4) {
                if (++consecutive == 4) {
                    System.out.println(n - 3);
                    return;
                }
            } else {
                consecutive = 0;
            }
        }
    }

    private static int distinctPrimeFactorCount(int value) {
        int count = 0;
        for (int factor = 2; factor * factor <= value; factor++) {
            if (value % factor == 0) {
                count++;
                while (value % factor == 0) value /= factor;
            }
        }
        return value > 1 ? count + 1 : count;
    }
}
