/** Project Euler 085: counting rectangles nearest to two million. */
public class Problem085 {
    public static void main(String[] args) {
        final int target = 2_000_000;
        int bestArea = 0, bestDifference = Integer.MAX_VALUE;
        for (int width = 1; width <= 2_000; width++) {
            long horizontal = (long) width * (width + 1) / 2;
            for (int height = 1; height <= width; height++) {
                long rectangles = horizontal * height * (height + 1) / 2;
                int difference = (int) Math.abs(rectangles - target);
                if (difference < bestDifference) { bestDifference = difference; bestArea = width * height; }
                if (rectangles > target) break;
            }
        }
        System.out.println(bestArea);
    }
}
