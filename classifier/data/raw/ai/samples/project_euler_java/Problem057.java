// Project Euler 57: square-root-two expansions with a longer numerator.
import java.math.BigInteger;

public class Problem057 {
    public static void main(String[] args) {
        BigInteger numerator = BigInteger.valueOf(3);
        BigInteger denominator = BigInteger.valueOf(2);
        int count = 0;
        for (int expansion = 1; expansion <= 1_000; expansion++) {
            if (numerator.toString().length() > denominator.toString().length()) count++;
            BigInteger nextNumerator = numerator.add(denominator.shiftLeft(1));
            denominator = numerator.add(denominator);
            numerator = nextNumerator;
        }
        System.out.println(count);
    }
}
