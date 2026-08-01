import java.math.BigInteger;

// Project Euler Problem 15: paths through a 20 by 20 lattice.
public class Problem015 {
    public static void main(String[] args) {
        BigInteger paths = BigInteger.ONE;
        for (int i = 1; i <= 20; i++) {
            paths = paths.multiply(BigInteger.valueOf(20 + i)).divide(BigInteger.valueOf(i));
        }
        System.out.println(paths);
    }
}
