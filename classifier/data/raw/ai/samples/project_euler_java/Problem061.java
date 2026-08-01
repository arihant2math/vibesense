// Project Euler 61: cyclic four-digit polygonal numbers.
import java.util.*;

public class Problem061 {
    static List<Integer>[] values = new List[6];
    static int[] path = new int[6];
    static boolean[] used = new boolean[6];

    public static void main(String[] args) {
        for (int type = 0; type < 6; type++) {
            values[type] = new ArrayList<>();
            int sides = type + 3;
            for (int n = 1;; n++) {
                int v = n * ((sides - 2) * n - (sides - 4)) / 2;
                if (v >= 10_000) break;
                if (v >= 1_000 && v % 100 >= 10) values[type].add(v);
            }
        }
        for (int first : values[0]) {
            path[0] = first;
            used[0] = true;
            if (search(1)) return;
            used[0] = false;
        }
    }

    static boolean search(int depth) {
        if (depth == 6) {
            if (path[5] % 100 == path[0] / 100) {
                int sum = 0;
                for (int v : path) sum += v;
                System.out.println(sum);
                return true;
            }
            return false;
        }
        int prefix = path[depth - 1] % 100;
        for (int type = 1; type < 6; type++) if (!used[type]) {
            for (int v : values[type]) if (v / 100 == prefix) {
                used[type] = true;
                path[depth] = v;
                if (search(depth + 1)) return true;
                used[type] = false;
            }
        }
        return false;
    }
}
