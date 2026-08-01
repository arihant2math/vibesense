// Project Euler 30: sum numbers equal to the sum of fifth powers of their digits.
public class Problem030 {
    public static void main(String[] args) {
        int[] fifthPower = new int[10];
        for (int d = 0; d < 10; d++) fifthPower[d] = d * d * d * d * d;

        int total = 0;
        // Seven digits cannot qualify: 7 * 9^5 has only six digits.
        for (int n = 2; n <= 6 * fifthPower[9]; n++) {
            int value = n;
            int sum = 0;
            while (value > 0) {
                sum += fifthPower[value % 10];
                value /= 10;
            }
            if (sum == n) total += n;
        }
        System.out.println(total);
    }
}
