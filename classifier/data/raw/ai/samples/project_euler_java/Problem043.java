// Project Euler Problem 43: substring-divisible pandigital numbers.
public class Problem043 {
    private static final int[] DIVISORS = {2, 3, 5, 7, 11, 13, 17};
    private static long sum;

    public static void main(String[] args) {
        build(0, 0, 0);
        System.out.println(sum);
    }

    private static void build(int position, int number, int used) {
        if (position == 10) {
            sum += number;
            return;
        }
        for (int digit = 0; digit <= 9; digit++) {
            if ((used & (1 << digit)) != 0 || (position == 0 && digit == 0)) continue;
            int next = number * 10 + digit;
            if (position >= 3 && next % 1000 % DIVISORS[position - 3] != 0) continue;
            build(position + 1, next, used | (1 << digit));
        }
    }
}
