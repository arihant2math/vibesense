// Project Euler 93: four digits producing the longest consecutive arithmetic run.
import java.util.*;
public class Problem093 {
    private static final class Fraction {
        final long p, q;
        Fraction(long p, long q) {
            if (q == 0) throw new ArithmeticException();
            if (q < 0) { p = -p; q = -q; }
            long g = gcd(Math.abs(p), q); this.p = p / g; this.q = q / g;
        }
        static long gcd(long a, long b) { while (b != 0) { long t = a % b; a = b; b = t; } return a; }
        Fraction add(Fraction x) { return new Fraction(p * x.q + x.p * q, q * x.q); }
        Fraction sub(Fraction x) { return new Fraction(p * x.q - x.p * q, q * x.q); }
        Fraction mul(Fraction x) { return new Fraction(p * x.p, q * x.q); }
        Fraction div(Fraction x) { return new Fraction(p * x.q, q * x.p); }
    }
    private static void evaluate(List<Fraction> values, Set<Integer> results) {
        if (values.size() == 1) {
            Fraction f = values.get(0);
            if (f.q == 1 && f.p > 0 && f.p <= Integer.MAX_VALUE) results.add((int) f.p);
            return;
        }
        for (int i = 0; i < values.size(); i++) for (int j = 0; j < values.size(); j++) if (i != j) {
            List<Fraction> rest = new ArrayList<>();
            for (int k = 0; k < values.size(); k++) if (k != i && k != j) rest.add(values.get(k));
            Fraction a = values.get(i), b = values.get(j);
            Fraction[] choices = {a.add(b), a.sub(b), a.mul(b)};
            for (Fraction f : choices) { rest.add(f); evaluate(rest, results); rest.remove(rest.size() - 1); }
            if (b.p != 0) { rest.add(a.div(b)); evaluate(rest, results); rest.remove(rest.size() - 1); }
        }
    }
    public static void main(String[] args) {
        int best = 0; String digits = "";
        for (int a = 1; a <= 6; a++) for (int b = a + 1; b <= 7; b++)
            for (int c = b + 1; c <= 8; c++) for (int d = c + 1; d <= 9; d++) {
                Set<Integer> results = new HashSet<>();
                evaluate(Arrays.asList(new Fraction(a,1), new Fraction(b,1), new Fraction(c,1), new Fraction(d,1)), results);
                int run = 1; while (results.contains(run)) run++;
                if (run - 1 > best) { best = run - 1; digits = "" + a + b + c + d; }
            }
        System.out.println(digits);
    }
}
