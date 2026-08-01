import java.math.BigInteger;

// Project Euler 25: index of the first Fibonacci term with 1,000 digits.
public class Problem025 {
    public static void main(String[] args) {
        BigInteger previous = BigInteger.ONE;
        BigInteger current = BigInteger.ONE;
        int index = 2;
        while (current.toString().length() < 1_000) {
            BigInteger next = previous.add(current);
            previous = current;
            current = next;
            index++;
        }
        System.out.println(index);
    }
}
