// Project Euler 52: smallest integer whose first six multiples are digit permutations.
import java.util.Arrays;

public class Problem052 {
    public static void main(String[] args) {
        for (int n = 1; ; n++) {
            char[] digits = Integer.toString(n).toCharArray();
            Arrays.sort(digits);
            boolean matches = true;
            for (int multiplier = 2; multiplier <= 6; multiplier++) {
                char[] other = Integer.toString(n * multiplier).toCharArray();
                Arrays.sort(other);
                if (!Arrays.equals(digits, other)) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                System.out.println(n);
                return;
            }
        }
    }
}
