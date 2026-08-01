/** Project Euler 36: sum numbers below one million palindromic in base 10 and base 2. */
public class Problem036 {
    public static void main(String[] args) {
        int sum = 0;
        for (int n = 1; n < 1_000_000; n++) {
            if (isPalindrome(Integer.toString(n))
                    && isPalindrome(Integer.toBinaryString(n))) {
                sum += n;
            }
        }
        System.out.println(sum);
    }

    private static boolean isPalindrome(String value) {
        for (int left = 0, right = value.length() - 1; left < right; left++, right--) {
            if (value.charAt(left) != value.charAt(right)) {
                return false;
            }
        }
        return true;
    }
}
