/** Project Euler Problem 6: difference between the square of sums and sum of squares through 100. */
public class Problem006 {
    public static void main(String[] args) {
        long sum = 0;
        long sumOfSquares = 0;
        for (int n = 1; n <= 100; n++) {
            sum += n;
            sumOfSquares += (long) n * n;
        }
        System.out.println(sum * sum - sumOfSquares);
    }
}
