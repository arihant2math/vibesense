import java.math.BigInteger;

// Project Euler Problem 20: sum of the decimal digits of 100 factorial.
public class Problem020 {
    public static void main(String[] args) {
        BigInteger factorial = BigInteger.ONE;
        for (int factor = 2; factor <= 100; factor++) factorial = factorial.multiply(BigInteger.valueOf(factor));
        int sum = 0;
        for (char digit : factorial.toString().toCharArray()) sum += digit - '0';
        System.out.println(sum);
    }
}
