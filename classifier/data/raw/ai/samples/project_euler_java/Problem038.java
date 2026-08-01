/** Project Euler 38: largest 1-to-9 pandigital concatenated product. */
public class Problem038 {
    public static void main(String[] args) {
        int largest = 0;
        for (int n = 1; n < 10_000; n++) {
            StringBuilder concatenated = new StringBuilder();
            for (int multiplier = 1; concatenated.length() < 9; multiplier++) {
                concatenated.append(n * multiplier);
            }
            if (concatenated.length() == 9 && isPandigital(concatenated)) {
                largest = Math.max(largest, Integer.parseInt(concatenated.toString()));
            }
        }
        System.out.println(largest);
    }

    private static boolean isPandigital(CharSequence value) {
        int mask = 0;
        for (int i = 0; i < value.length(); i++) {
            int digit = value.charAt(i) - '0';
            if (digit == 0 || (mask & (1 << digit)) != 0) {
                return false;
            }
            mask |= 1 << digit;
        }
        return mask == 0x3FE;
    }
}
