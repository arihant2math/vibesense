// Project Euler 68: maximum 16-digit string for a magic 5-gon ring.
import java.util.*;

public class Problem068 {
    static String best = "";

    public static void main(String[] args) {
        int[] inner = new int[5];
        chooseInner(0, 0, inner);
        System.out.println(best);
    }

    static void chooseInner(int position, int mask, int[] inner) {
        if (position == 5) { evaluate(inner, mask); return; }
        for (int n = 1; n <= 9; n++) if ((mask & (1 << n)) == 0) {
            inner[position] = n;
            chooseInner(position + 1, mask | (1 << n), inner);
        }
    }

    static void evaluate(int[] in, int used) {
        for (int firstOuter = 1; firstOuter <= 10; firstOuter++) if ((used & (1 << firstOuter)) == 0) {
            int total = firstOuter + in[0] + in[1];
            int[] outer = new int[5];
            outer[0] = firstOuter;
            int outerMask = used | (1 << firstOuter);
            boolean valid = true;
            for (int i = 1; i < 5; i++) {
                int value = total - in[i] - in[(i + 1) % 5];
                if (value < 1 || value > 10 || (outerMask & (1 << value)) != 0) { valid = false; break; }
                outer[i] = value;
                outerMask |= 1 << value;
            }
            if (!valid || outerMask != ((1 << 11) - 2)) continue;
            for (int i = 1; i < 5; i++) if (outer[i] < outer[0]) return;
            StringBuilder s = new StringBuilder();
            for (int i = 0; i < 5; i++) s.append(outer[i]).append(in[i]).append(in[(i + 1) % 5]);
            if (s.length() == 16 && s.toString().compareTo(best) > 0) best = s.toString();
        }
    }
}
