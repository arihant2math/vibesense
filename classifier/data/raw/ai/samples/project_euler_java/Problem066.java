// Project Euler 66: D <= 1000 with the largest minimal Pell-equation x.
import java.math.BigInteger;

public class Problem066 {
    public static void main(String[] args) {
        BigInteger largestX = BigInteger.ZERO;
        int answer = 0;
        for (int d = 2; d <= 1000; d++) {
            int root = (int) Math.sqrt(d);
            if (root * root == d) continue;
            int m = 0, q = 1, a = root;
            BigInteger pMinus2 = BigInteger.ZERO, pMinus1 = BigInteger.ONE;
            BigInteger qMinus2 = BigInteger.ONE, qMinus1 = BigInteger.ZERO;
            while (true) {
                BigInteger p = BigInteger.valueOf(a).multiply(pMinus1).add(pMinus2);
                BigInteger den = BigInteger.valueOf(a).multiply(qMinus1).add(qMinus2);
                if (p.multiply(p).subtract(den.multiply(den).multiply(BigInteger.valueOf(d))).equals(BigInteger.ONE)) {
                    if (p.compareTo(largestX) > 0) { largestX = p; answer = d; }
                    break;
                }
                pMinus2 = pMinus1; pMinus1 = p;
                qMinus2 = qMinus1; qMinus1 = den;
                m = q * a - m;
                q = (d - m * m) / q;
                a = (root + m) / q;
            }
        }
        System.out.println(answer);
    }
}
