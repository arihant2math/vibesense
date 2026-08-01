/** Project Euler Problem 4: largest palindrome made from two three-digit numbers. */
public class Problem004 {
    public static void main(String[] args) {
        int largest = 0;
        for (int a = 999; a >= 100; a--) {
            for (int b = a; b >= 100; b--) {
                int product = a * b;
                if (product <= largest) break;
                if (isPalindrome(product)) largest = product;
            }
        }
        System.out.println(largest);
    }

    private static boolean isPalindrome(int value) {
        int original = value;
        int reversed = 0;
        while (value > 0) {
            reversed = reversed * 10 + value % 10;
            value /= 10;
        }
        return original == reversed;
    }
}
