// Project Euler 62: smallest cube with exactly five cubic permutations.
import java.util.*;

public class Problem062 {
    public static void main(String[] args) {
        Map<String, List<Long>> groups = new HashMap<>();
        int digits = 1;
        for (long n = 1;; n++) {
            long cube = n * n * n;
            int length = Long.toString(cube).length();
            if (length != digits) {
                long answer = Long.MAX_VALUE;
                for (List<Long> group : groups.values())
                    if (group.size() == 5) answer = Math.min(answer, group.get(0));
                if (answer != Long.MAX_VALUE) {
                    System.out.println(answer);
                    return;
                }
                groups.clear();
                digits = length;
            }
            char[] chars = Long.toString(cube).toCharArray();
            Arrays.sort(chars);
            groups.computeIfAbsent(new String(chars), k -> new ArrayList<>()).add(cube);
        }
    }
}
