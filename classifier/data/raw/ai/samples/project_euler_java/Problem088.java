import java.util.Arrays;

/** Project Euler 088: minimal product-sum numbers for 2 <= k <= 12000. */
public class Problem088 {
    private static final int MAX_K = 12_000;
    private static final int[] minimum = new int[MAX_K + 1];

    public static void main(String[] args) {
        Arrays.fill(minimum, Integer.MAX_VALUE);
        // A minimal product-sum number for k cannot exceed 2k.
        search(2, 1, 0, 0);
        boolean[] seen = new boolean[MAX_K * 2 + 1];
        int answer = 0;
        for (int k = 2; k <= MAX_K; k++) if (!seen[minimum[k]]) {
            seen[minimum[k]] = true;
            answer += minimum[k];
        }
        System.out.println(answer);
    }

    // Factors are nondecreasing, so every multiplicative partition is considered once.
    private static void search(int factor, int product, int sum, int factors) {
        int k = product - sum + factors; // append (product - sum) ones if needed
        if (k <= MAX_K && product < minimum[k]) minimum[k] = product;
        for (int next = factor; product * next <= 2 * MAX_K; next++)
            search(next, product * next, sum + next, factors + 1);
    }
}
