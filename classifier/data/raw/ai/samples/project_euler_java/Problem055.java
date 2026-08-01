// Project Euler 55: count Lychrel-number candidates below ten thousand.
import java.math.BigInteger;

public class Problem055 {
    public static void main(String[] args) {
        int count = 0;
        for (int n = 1; n < 10_000; n++) {
            BigInteger value = BigInteger.valueOf(n);
            boolean palindromeFound = false;
            for (int iteration = 0; iteration < 50; iteration++) {
                value = value.add(reverse(value));
                if (isPalindrome(value)) {
                    palindromeFound = true;
                    break;
                }
            }
            if (!palindromeFound) count++;
        }
        System.out.println(count);
    }

    private static BigInteger reverse(BigInteger value) {
        return new BigInteger(new StringBuilder(value.toString()).reverse().toString());
    }

    private static boolean isPalindrome(BigInteger value) {
        String text = value.toString();
        return text.contentEquals(new StringBuilder(text).reverse());
    }
}
