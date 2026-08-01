import java.math.BigInteger;

// Project Euler Problem 16: sum of the decimal digits of 2 raised to the 1000th power.
public class Problem016 {
    public static void main(String[] args) {
        String power = BigInteger.TWO.pow(1000).toString();
        int sum = 0;
        for (int i = 0; i < power.length(); i++) sum += power.charAt(i) - '0';
        System.out.println(sum);
    }
}
