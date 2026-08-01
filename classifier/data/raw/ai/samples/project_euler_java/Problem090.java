import java.util.ArrayList;
import java.util.List;

/** Project Euler 090: distinct arrangements of two cube digit sets. */
public class Problem090 {
    private static final int[][] SQUARES = {
        {0, 1}, {0, 4}, {0, 9}, {1, 6}, {2, 5}, {3, 6}, {4, 9}, {6, 4}, {8, 1}
    };

    public static void main(String[] args) {
        List<Integer> dice = new ArrayList<>();
        choose(dice, 0, 0, 0);
        int answer = 0;
        for (int i = 0; i < dice.size(); i++) for (int j = i; j < dice.size(); j++)
            if (canDisplayAll(dice.get(i), dice.get(j))) answer++;
        System.out.println(answer);
    }

    private static void choose(List<Integer> dice, int nextDigit, int chosen, int mask) {
        if (chosen == 6) { dice.add(mask); return; }
        for (int digit = nextDigit; digit <= 10 - (6 - chosen); digit++)
            choose(dice, digit + 1, chosen + 1, mask | (1 << digit));
    }

    private static boolean canDisplayAll(int first, int second) {
        // Rotating a die makes 6 and 9 interchangeable.
        if ((first & ((1 << 6) | (1 << 9))) != 0) first |= (1 << 6) | (1 << 9);
        if ((second & ((1 << 6) | (1 << 9))) != 0) second |= (1 << 6) | (1 << 9);
        for (int[] pair : SQUARES) {
            int a = 1 << pair[0], b = 1 << pair[1];
            if (!((first & a) != 0 && (second & b) != 0 || (first & b) != 0 && (second & a) != 0))
                return false;
        }
        return true;
    }
}
