// Project Euler 97: last ten digits of the non-Mersenne prime.
import java.math.BigInteger;
public class Problem097 {
    public static void main(String[] args) {
        BigInteger modulus = BigInteger.TEN.pow(10);
        BigInteger power = BigInteger.valueOf(2).modPow(BigInteger.valueOf(7_830_457), modulus);
        System.out.println(power.multiply(BigInteger.valueOf(28_433)).add(BigInteger.ONE).mod(modulus));
    }
}
