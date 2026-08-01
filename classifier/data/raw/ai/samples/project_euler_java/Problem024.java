// Project Euler 24: the millionth lexicographic permutation of 0 through 9.
public class Problem024 {
    public static void main(String[] args) {
        long index = 1_000_000L - 1; // zero-based rank
        long[] factorial = new long[11];
        factorial[0] = 1;
        for (int i = 1; i <= 10; i++) factorial[i] = factorial[i - 1] * i;

        StringBuilder available = new StringBuilder("0123456789");
        StringBuilder result = new StringBuilder();
        for (int remaining = 10; remaining > 0; remaining--) {
            int choice = (int) (index / factorial[remaining - 1]);
            result.append(available.charAt(choice));
            available.deleteCharAt(choice);
            index %= factorial[remaining - 1];
        }
        System.out.println(result);
    }
}
