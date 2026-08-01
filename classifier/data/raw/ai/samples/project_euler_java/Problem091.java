// Project Euler 91: count right triangles in a 50 by 50 lattice.
public class Problem091 {
    public static void main(String[] args) {
        final int n = 50;
        long count = 0;
        for (int ax = 0; ax <= n; ax++) for (int ay = 0; ay <= n; ay++) {
            if (ax == 0 && ay == 0) continue;
            for (int bx = 0; bx <= n; bx++) for (int by = 0; by <= n; by++) {
                if (bx == 0 && by == 0 || (ax == bx && ay == by)) continue;
                long atOrigin = (long) ax * bx + (long) ay * by;
                long atA = (long) -ax * (bx - ax) + (long) -ay * (by - ay);
                long atB = (long) -bx * (ax - bx) + (long) -by * (ay - by);
                if (atOrigin == 0 || atA == 0 || atB == 0) count++;
            }
        }
        // Each unordered pair of non-origin vertices was visited in both orders.
        System.out.println(count / 2);
    }
}
