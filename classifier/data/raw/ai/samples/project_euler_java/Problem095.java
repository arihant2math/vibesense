// Project Euler 95: smallest member of the longest amicable chain below one million.
public class Problem095 {
    public static void main(String[] args) {
        final int limit = 1_000_000;
        int[] sum = new int[limit + 1];
        for (int d = 1; d <= limit / 2; d++) for (int k = d + d; k <= limit; k += d) sum[k] += d;
        boolean[] processed = new boolean[limit + 1];
        int[] visit = new int[limit + 1], path = new int[limit + 1];
        int bestLength = 0, bestMember = 0;
        for (int start = 2, run = 1; start <= limit; start++, run++) {
            if (processed[start]) continue;
            int x = start, size = 0;
            while (x <= limit && !processed[x] && visit[x] != run) {
                visit[x] = run; path[size++] = x; x = sum[x];
            }
            if (x <= limit && visit[x] == run) {
                int begin = 0; while (path[begin] != x) begin++;
                int length = size - begin, smallest = path[begin];
                for (int i = begin + 1; i < size; i++) smallest = Math.min(smallest, path[i]);
                if (length > bestLength) { bestLength = length; bestMember = smallest; }
            }
            for (int i = 0; i < size; i++) processed[path[i]] = true;
        }
        System.out.println(bestMember);
    }
}
