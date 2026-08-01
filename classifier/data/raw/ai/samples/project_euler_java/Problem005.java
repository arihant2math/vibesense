/** Project Euler Problem 5: smallest positive number divisible by every integer from 1 to 20. */
public class Problem005 {
    public static void main(String[] args) {
        long multiple = 1;
        for (int n = 2; n <= 20; n++) {
            multiple = lcm(multiple, n);
        }
        System.out.println(multiple);
    }

    private static long lcm(long a, long b) {
        return a / gcd(a, b) * b;
    }

    private static long gcd(long a, long b) {
        while (b != 0) {
            long remainder = a % b;
            a = b;
            b = remainder;
        }
        return a;
    }
}
