// Project Euler 65: digit sum of the numerator of e's 100th convergent.
import java.math.BigInteger;

public class Problem065 {
    public static void main(String[] args) {
        BigInteger previousNumerator = BigInteger.ZERO;
        BigInteger numerator = BigInteger.ONE;
        BigInteger previousDenominator = BigInteger.ONE;
        BigInteger denominator = BigInteger.ZERO;
        for (int i = 0; i < 100; i++) {
            int coefficient = i == 0 ? 2 : (i % 3 == 2 ? 2 * ((i + 1) / 3) : 1);
            BigInteger nextNumerator = numerator.multiply(BigInteger.valueOf(coefficient)).add(previousNumerator);
            BigInteger nextDenominator = denominator.multiply(BigInteger.valueOf(coefficient)).add(previousDenominator);
            previousNumerator = numerator;
            numerator = nextNumerator;
            previousDenominator = denominator;
            denominator = nextDenominator;
        }
        int sum = 0;
        for (char c : numerator.toString().toCharArray()) sum += c - '0';
        System.out.println(sum);
    }
}
