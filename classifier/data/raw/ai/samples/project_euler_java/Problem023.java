// Project Euler 23: sum numbers not expressible as two abundant numbers.
public class Problem023 {
    private static final int LIMIT = 28_123;

    public static void main(String[] args) {
        int[] divisorSums = new int[LIMIT + 1];
        for (int d = 1; d <= LIMIT / 2; d++)
            for (int n = d * 2; n <= LIMIT; n += d) divisorSums[n] += d;

        int[] abundant = new int[LIMIT];
        int count = 0;
        for (int n = 12; n <= LIMIT; n++)
            if (divisorSums[n] > n) abundant[count++] = n;

        boolean[] expressible = new boolean[LIMIT + 1];
        for (int i = 0; i < count; i++) {
            for (int j = i; j < count; j++) {
                int sum = abundant[i] + abundant[j];
                if (sum > LIMIT) break;
                expressible[sum] = true;
            }
        }
        int answer = 0;
        for (int n = 1; n <= LIMIT; n++) if (!expressible[n]) answer += n;
        System.out.println(answer);
    }
}
