// Project Euler 76: partitions of 100 using at least two positive integers.
public class Problem076 {
    public static void main(String[] args) {
        final int target = 100;
        long[] ways = new long[target + 1];
        ways[0] = 1;
        for (int part = 1; part < target; part++) {
            for (int sum = part; sum <= target; sum++) {
                ways[sum] += ways[sum - part];
            }
        }
        System.out.println(ways[target]);
    }
}
