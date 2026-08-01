// Project Euler 78: first partition number divisible by one million.
public class Problem078 {
    public static void main(String[] args) {
        final int modulus = 1_000_000;
        int[] partitions = new int[60_000];
        partitions[0] = 1;
        for (int n = 1; n < partitions.length; n++) {
            int value = 0;
            for (int k = 1; ; k++) {
                int first = k * (3 * k - 1) / 2;
                if (first > n) {
                    break;
                }
                int sign = (k & 1) == 1 ? 1 : -1;
                value = (value + sign * partitions[n - first]) % modulus;
                int second = k * (3 * k + 1) / 2;
                if (second <= n) {
                    value = (value + sign * partitions[n - second]) % modulus;
                }
            }
            if (value < 0) {
                value += modulus;
            }
            partitions[n] = value;
            if (value == 0) {
                System.out.println(n);
                return;
            }
        }
    }
}
