/** Project Euler 31: number of ways to make £2 using British coins. */
public class Problem031 {
    public static void main(String[] args) {
        int[] coins = {1, 2, 5, 10, 20, 50, 100, 200};
        int[] ways = new int[201];
        ways[0] = 1;
        for (int coin : coins) {
            for (int amount = coin; amount <= 200; amount++) {
                ways[amount] += ways[amount - coin];
            }
        }
        System.out.println(ways[200]);
    }
}
