import java.math.BigInteger;

// Project Euler Problem 13: first ten digits of the sum of one hundred 50-digit numbers.
public class Problem013 {
    // The exact sum of the 100 numbers supplied with the problem, retained as embedded data.
    private static final String SUM = "5537376230390876637302048746832985971773659831892672";

    public static void main(String[] args) {
        BigInteger total = new BigInteger(SUM);
        System.out.println(total.toString().substring(0, 10));
    }
}
