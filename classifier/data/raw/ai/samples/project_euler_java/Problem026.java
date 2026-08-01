// Project Euler 26: d < 1000 whose reciprocal has the longest recurring cycle.
public class Problem026 {
    private static int cycleLength(int denominator) {
        int[] firstSeen = new int[denominator];
        java.util.Arrays.fill(firstSeen, -1);
        int remainder = 1 % denominator;
        int position = 0;
        while (remainder != 0 && firstSeen[remainder] == -1) {
            firstSeen[remainder] = position++;
            remainder = (remainder * 10) % denominator;
        }
        return remainder == 0 ? 0 : position - firstSeen[remainder];
    }

    public static void main(String[] args) {
        int bestDenominator = 0;
        int bestLength = 0;
        for (int d = 2; d < 1_000; d++) {
            int length = cycleLength(d);
            if (length > bestLength) {
                bestLength = length;
                bestDenominator = d;
            }
        }
        System.out.println(bestDenominator);
    }
}
