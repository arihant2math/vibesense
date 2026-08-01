/** Project Euler 34: sum of numbers equal to the sum of the factorials of their digits. */
public class Problem034 {
    public static void main(String[] args) {
        int[] factorial = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};
        int sum = 0;
        for (int n = 10; n <= 7 * factorial[9]; n++) {
            int value = n;
            int factorialSum = 0;
            while (value > 0) {
                factorialSum += factorial[value % 10];
                value /= 10;
            }
            if (factorialSum == n) {
                sum += n;
            }
        }
        System.out.println(sum);
    }
}
