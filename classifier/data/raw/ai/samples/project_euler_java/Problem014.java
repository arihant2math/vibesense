// Project Euler Problem 14: starting number below one million with the longest Collatz chain.
public class Problem014 {
    private static final int LIMIT = 1_000_000;

    public static void main(String[] args) {
        int[] lengths = new int[5_000_000];
        lengths[1] = 1;
        int bestStart = 1, bestLength = 1;
        for (int start = 2; start < LIMIT; start++) {
            long value = start;
            int steps = 0;
            while (value >= lengths.length || lengths[(int) value] == 0) {
                value = (value & 1) == 0 ? value / 2 : 3 * value + 1;
                steps++;
            }
            int length = steps + lengths[(int) value];
            if (length > bestLength) {
                bestLength = length;
                bestStart = start;
            }
            value = start;
            while (value < lengths.length && lengths[(int) value] == 0) {
                lengths[(int) value] = length--;
                value = (value & 1) == 0 ? value / 2 : 3 * value + 1;
            }
        }
        System.out.println(bestStart);
    }
}
