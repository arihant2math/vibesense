// Project Euler 51: smallest prime in an eight-prime digit-replacement family.
public class Problem051 {
    public static void main(String[] args) {
        for (int n = 11; ; n += 2) {
            if (!isPrime(n)) continue;
            String s = Integer.toString(n);
            int masks = 1 << s.length();
            for (int mask = 1; mask < masks; mask++) {
                int count = 0;
                int smallest = 0;
                for (int digit = 0; digit <= 9; digit++) {
                    if ((mask & 1) != 0 && digit == 0) continue;
                    char[] value = s.toCharArray();
                    for (int i = 0; i < value.length; i++) {
                        if ((mask & (1 << i)) != 0) value[i] = (char) ('0' + digit);
                    }
                    int candidate = Integer.parseInt(new String(value));
                    if (isPrime(candidate)) {
                        count++;
                        if (smallest == 0) smallest = candidate;
                    }
                }
                if (count >= 8) {
                    System.out.println(smallest);
                    return;
                }
            }
        }
    }

    private static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n % 2 == 0) return n == 2;
        for (int d = 3; d <= n / d; d += 2) {
            if (n % d == 0) return false;
        }
        return true;
    }
}
