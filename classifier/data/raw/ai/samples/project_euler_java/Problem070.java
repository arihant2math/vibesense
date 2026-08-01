// Project Euler 70: totient permutation minimizing n / phi(n) below ten million.

public class Problem070 {
    public static void main(String[] args) {
        final int limit = 10_000_000;
        int[] phi = new int[limit];
        for (int i = 0; i < limit; i++) phi[i] = i;
        for (int p = 2; p < limit; p++) if (phi[p] == p) {
            for (int n = p; n < limit; n += p) phi[n] -= phi[n] / p;
        }
        int answer = 0, bestPhi = 1;
        for (int n = 2; n < limit; n++) {
            if (sameDigits(n, phi[n]) && (answer == 0 || (long) n * bestPhi < (long) answer * phi[n])) {
                answer = n;
                bestPhi = phi[n];
            }
        }
        System.out.println(answer);
    }

    static boolean sameDigits(int a, int b) {
        int[] counts = new int[10];
        do { counts[a % 10]++; a /= 10; } while (a != 0);
        do { counts[b % 10]--; b /= 10; } while (b != 0);
        for (int count : counts) if (count != 0) return false;
        return true;
    }
}
