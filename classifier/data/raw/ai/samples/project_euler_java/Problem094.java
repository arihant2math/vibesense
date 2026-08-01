// Project Euler 94: sum perimeters of almost-equilateral integral-area triangles.
public class Problem094 {
    public static void main(String[] args) {
        long m = 2, n = 0, total = 0;
        while (true) {
            long nextM = 2 * m + 3 * n, nextN = m + 2 * n; // multiply by 2 + sqrt(3)
            m = nextM; n = nextN; // m*m - 3*n*n = 4
            long side, base;
            if ((m + 1) % 3 == 0) { side = (m + 1) / 3; base = side + 1; }
            else { side = (m - 1) / 3; base = side - 1; }
            long perimeter = 2 * side + base;
            if (perimeter >= 1_000_000_000L) break;
            if (side > 1) total += perimeter;
        }
        System.out.println(total);
    }
}
