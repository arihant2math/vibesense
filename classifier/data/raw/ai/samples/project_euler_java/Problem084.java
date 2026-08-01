import java.util.Arrays;

/** Project Euler 084: Monopoly odds with two four-sided dice. */
public class Problem084 {
    private static final int GO = 0, JAIL = 10, G2J = 30;
    private static final int[] CHANCE = {7, 22, 36};
    private static final int[] CHEST = {2, 17, 33};

    public static void main(String[] args) {
        // A state includes the current square and the current run of doubles.
        double[][] transition = new double[120][120];
        for (int square = 0; square < 40; square++) for (int doubles = 0; doubles < 3; doubles++) {
            int from = state(square, doubles);
            for (int a = 1; a <= 4; a++) for (int b = 1; b <= 4; b++) {
                double probability = 1.0 / 16.0;
                if (a == b && doubles == 2) {
                    transition[from][state(JAIL, 0)] += probability;
                } else {
                    int nextDoubles = a == b ? doubles + 1 : 0;
                    land(transition, from, (square + a + b) % 40, nextDoubles, probability);
                }
            }
        }

        double[] distribution = new double[120];
        distribution[state(GO, 0)] = 1;
        for (int iteration = 0; iteration < 10_000; iteration++) {
            double[] next = new double[120];
            for (int from = 0; from < 120; from++) if (distribution[from] != 0)
                for (int to = 0; to < 120; to++) next[to] += distribution[from] * transition[from][to];
            distribution = next;
        }
        double[] squares = new double[40];
        for (int square = 0; square < 40; square++)
            for (int doubles = 0; doubles < 3; doubles++) squares[square] += distribution[state(square, doubles)];
        Integer[] order = new Integer[40];
        for (int i = 0; i < 40; i++) order[i] = i;
        Arrays.sort(order, (x, y) -> Double.compare(squares[y], squares[x]));
        System.out.printf("%02d%02d%02d%n", order[0], order[1], order[2]);
    }

    private static int state(int square, int doubles) { return square * 3 + doubles; }

    private static void land(double[][] transition, int from, int square, int doubles, double probability) {
        if (square == G2J) { transition[from][state(JAIL, 0)] += probability; return; }
        if (contains(CHEST, square)) {
            // Community Chest: GO, JAIL, or one of fourteen cards which do not move us.
            transition[from][state(GO, doubles)] += probability / 16;
            transition[from][state(JAIL, 0)] += probability / 16;
            transition[from][state(square, doubles)] += probability * 14 / 16;
        } else if (contains(CHANCE, square)) {
            // The six non-moving Chance cards are combined into one branch.
            transition[from][state(square, doubles)] += probability * 6 / 16;
            moveCard(transition, from, GO, doubles, probability / 16);
            moveCard(transition, from, JAIL, doubles, probability / 16);
            moveCard(transition, from, 11, doubles, probability / 16); // C1
            moveCard(transition, from, 24, doubles, probability / 16); // E3
            moveCard(transition, from, 39, doubles, probability / 16); // H2
            moveCard(transition, from, 5, doubles, probability / 16);  // R1
            moveCard(transition, from, nextRailroad(square), doubles, probability * 2 / 16);
            moveCard(transition, from, nextUtility(square), doubles, probability / 16);
            land(transition, from, (square + 37) % 40, doubles, probability / 16); // back three
        } else {
            transition[from][state(square, doubles)] += probability;
        }
    }

    private static void moveCard(double[][] t, int from, int square, int doubles, double p) {
        t[from][state(square, square == JAIL ? 0 : doubles)] += p;
    }
    private static int nextRailroad(int square) {
        if (square == 7) return 15;
        if (square == 22) return 25;
        return 5;
    }
    private static int nextUtility(int square) { return square == 22 ? 28 : 12; }
    private static boolean contains(int[] values, int value) {
        for (int item : values) if (item == value) return true;
        return false;
    }
}
