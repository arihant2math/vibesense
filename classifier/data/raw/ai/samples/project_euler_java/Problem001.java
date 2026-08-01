/** Project Euler Problem 1: multiples of 3 or 5 below 1000. */
public class Problem001 {
    public static void main(String[] args) {
        long sum = 0;
        for (int n = 1; n < 1000; n++) {
            if (n % 3 == 0 || n % 5 == 0) sum += n;
        }
        System.out.println(sum);
    }
}
