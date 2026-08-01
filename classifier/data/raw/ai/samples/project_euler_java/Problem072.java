// Project Euler 72: count reduced proper fractions with denominator at most one million.
public class Problem072 {
    public static void main(String[] args) {
        final int limit = 1_000_000;
        int[] phi = new int[limit + 1];
        for (int i = 0; i <= limit; i++) {
            phi[i] = i;
        }
        for (int p = 2; p <= limit; p++) {
            if (phi[p] == p) {
                for (int multiple = p; multiple <= limit; multiple += p) {
                    phi[multiple] -= phi[multiple] / p;
                }
            }
        }
        long count = 0;
        for (int d = 2; d <= limit; d++) {
            count += phi[d];
        }
        System.out.println(count);
    }
}
