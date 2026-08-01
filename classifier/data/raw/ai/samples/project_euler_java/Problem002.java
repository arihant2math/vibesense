/** Project Euler Problem 2: even Fibonacci terms not exceeding four million. */
public class Problem002 {
    public static void main(String[] args) {
        long sum = 0;
        int previous = 1;
        int current = 2;
        while (current <= 4_000_000) {
            if (current % 2 == 0) sum += current;
            int next = previous + current;
            previous = current;
            current = next;
        }
        System.out.println(sum);
    }
}
