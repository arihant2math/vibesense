/** Project Euler 086: integer shortest paths for cuboids. */
public class Problem086 {
    public static void main(String[] args) {
        int count = 0;
        for (int largest = 1; ; largest++) {
            for (int sum = 2; sum <= 2 * largest; sum++) {
                int squared = largest * largest + sum * sum;
                int root = (int) Math.sqrt(squared);
                if (root * root != squared) continue;
                // Count 1 <= a <= b <= largest such that a + b == sum.
                int low = Math.max(1, sum - largest);
                int high = Math.min(largest, sum / 2);
                if (high >= low) count += high - low + 1;
            }
            if (count > 1_000_000) { System.out.println(largest); return; }
        }
    }
}
