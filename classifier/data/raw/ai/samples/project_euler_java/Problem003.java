/** Project Euler Problem 3: largest prime factor of 600851475143. */
public class Problem003 {
    public static void main(String[] args) {
        long value = 600_851_475_143L;
        long largest = 1;
        for (long factor = 2; factor * factor <= value; factor += factor == 2 ? 1 : 2) {
            while (value % factor == 0) {
                largest = factor;
                value /= factor;
            }
        }
        if (value > 1) largest = value;
        System.out.println(largest);
    }
}
