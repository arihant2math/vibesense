import java.math.BigInteger;
import java.util.HashSet;
import java.util.Set;

// Project Euler 29: count distinct terms a^b for 2 <= a,b <= 100.
public class Problem029 {
    public static void main(String[] args) {
        Set<BigInteger> terms = new HashSet<>();
        for (int a = 2; a <= 100; a++) {
            BigInteger base = BigInteger.valueOf(a);
            for (int b = 2; b <= 100; b++) terms.add(base.pow(b));
        }
        System.out.println(terms.size());
    }
}
