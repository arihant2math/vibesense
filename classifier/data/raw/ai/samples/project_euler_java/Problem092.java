// Project Euler 92: square-digit chains ending at 89 below ten million.
public class Problem092 {
    private static int next(int n) {
        int sum = 0;
        while (n != 0) { int d = n % 10; sum += d * d; n /= 10; }
        return sum;
    }
    public static void main(String[] args) {
        int[] endings = new int[568];
        endings[1] = 1; endings[89] = 89;
        for (int i = 2; i < endings.length; i++) {
            int x = i;
            while (endings[x] == 0) x = next(x);
            endings[i] = endings[x];
        }
        long[] counts = {1};
        for (int position = 0; position < 7; position++) {
            long[] expanded = new long[81 * (position + 1) + 1];
            for (int sum = 0; sum < counts.length; sum++)
                for (int digit = 0; digit <= 9; digit++) expanded[sum + digit * digit] += counts[sum];
            counts = expanded;
        }
        long answer = 0;
        for (int sum = 1; sum < counts.length; sum++) if (endings[sum] == 89) answer += counts[sum];
        System.out.println(answer); // 0..9,999,999; zero itself does not end at 89.
    }
}
