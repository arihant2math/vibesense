// Project Euler 28: diagonal sum in a 1001 by 1001 number spiral.
public class Problem028 {
    public static void main(String[] args) {
        long sum = 1;
        long corner = 1;
        for (int side = 3; side <= 1001; side += 2) {
            int step = side - 1;
            for (int i = 0; i < 4; i++) {
                corner += step;
                sum += corner;
            }
        }
        System.out.println(sum);
    }
}
