/** Project Euler Problem 10: sum of all primes below two million. */
public class Problem010 {
    public static void main(String[] args) {
        final int limit = 2_000_000;
        boolean[] composite = new boolean[limit];
        long sum = 0;
        for (int n = 2; n < limit; n++) {
            if (!composite[n]) {
                sum += n;
                if ((long) n * n < limit) {
                    for (int multiple = n * n; multiple < limit; multiple += n) {
                        composite[multiple] = true;
                    }
                }
            }
        }
        System.out.println(sum);
    }
}
