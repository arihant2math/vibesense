// Project Euler 100: arranged probability with more than one trillion total discs.
public class Problem100 {
    public static void main(String[] args) {
        // x = 2*total-1 and y = 2*blue-1 satisfy x^2 - 2y^2 = -1.
        long x = 7, y = 5;
        do {
            long nextX = 3 * x + 4 * y, nextY = 2 * x + 3 * y;
            x = nextX; y = nextY;
        } while ((x + 1) / 2 <= 1_000_000_000_000L);
        System.out.println((y + 1) / 2);
    }
}
