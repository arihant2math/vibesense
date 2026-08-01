/** Project Euler Problem 7: the 10001st prime number. */
public class Problem007 {
    public static void main(String[] args) {
        int count = 0;
        int candidate = 1;
        while (count < 10_001) {
            candidate++;
            if (isPrime(candidate)) count++;
        }
        System.out.println(candidate);
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n % 2 == 0) return n == 2;
        for (int divisor = 3; divisor <= n / divisor; divisor += 2) {
            if (n % divisor == 0) return false;
        }
        return true;
    }
}
