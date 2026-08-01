/** Project Euler 39: perimeter at most 1000 with the most integer right triangles. */
public class Problem039 {
    public static void main(String[] args) {
        int[] solutions = new int[1001];
        for (int a = 1; a <= 1000; a++) {
            for (int b = a; a + b <= 1000; b++) {
                int cSquared = a * a + b * b;
                int c = (int) Math.sqrt(cSquared);
                if (c * c == cSquared && a + b + c <= 1000) {
                    solutions[a + b + c]++;
                }
            }
        }
        int bestPerimeter = 0;
        for (int perimeter = 1; perimeter <= 1000; perimeter++) {
            if (solutions[perimeter] > solutions[bestPerimeter]) {
                bestPerimeter = perimeter;
            }
        }
        System.out.println(bestPerimeter);
    }
}
