// Project Euler 74: digit-factorial chains of exactly sixty non-repeating terms.
import java.util.ArrayList;
import java.util.List;

public class Problem074 {
    private static final int[] FACTORIAL = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};
    private static final int MAX_VALUE = 3_000_000;

    public static void main(String[] args) {
        int[] length = new int[MAX_VALUE];
        int[] visited = new int[MAX_VALUE];
        int stamp = 0;
        int answer = 0;

        for (int start = 1; start < 1_000_000; start++) {
            stamp++;
            List<Integer> path = new ArrayList<>();
            int value = start;
            while (length[value] == 0 && visited[value] != stamp) {
                visited[value] = stamp;
                path.add(value);
                value = next(value);
            }

            if (length[value] != 0) {
                int known = length[value];
                for (int i = path.size() - 1; i >= 0; i--) {
                    length[path.get(i)] = ++known;
                }
            } else {
                int cycleStart = 0;
                while (path.get(cycleStart) != value) {
                    cycleStart++;
                }
                int cycleLength = path.size() - cycleStart;
                for (int i = cycleStart; i < path.size(); i++) {
                    length[path.get(i)] = cycleLength;
                }
                int known = cycleLength;
                for (int i = cycleStart - 1; i >= 0; i--) {
                    length[path.get(i)] = ++known;
                }
            }
            if (length[start] == 60) {
                answer++;
            }
        }
        System.out.println(answer);
    }

    private static int next(int value) {
        if (value == 0) {
            return 1;
        }
        int sum = 0;
        while (value > 0) {
            sum += FACTORIAL[value % 10];
            value /= 10;
        }
        return sum;
    }
}
