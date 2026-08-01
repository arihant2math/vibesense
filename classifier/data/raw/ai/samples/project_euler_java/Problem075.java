// Project Euler 75: perimeters with exactly one integral right-triangle solution.
public class Problem075 {
    public static void main(String[] args) {
        final int limit = 1_500_000;
        int[] solutions = new int[limit + 1];
        for (int m = 2; 2 * m * (m + 1) <= limit; m++) {
            for (int n = 1; n < m; n++) {
                if (((m - n) & 1) == 1 && gcd(m, n) == 1) {
                    int primitivePerimeter = 2 * m * (m + n);
                    for (int p = primitivePerimeter; p <= limit; p += primitivePerimeter) {
                        solutions[p]++;
                    }
                }
            }
        }
        int answer = 0;
        for (int p = 0; p <= limit; p++) {
            if (solutions[p] == 1) {
                answer++;
            }
        }
        System.out.println(answer);
    }

    private static int gcd(int a, int b) {
        while (b != 0) {
            int t = a % b;
            a = b;
            b = t;
        }
        return a;
    }
}
