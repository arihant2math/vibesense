/** Project Euler 40: product of selected digits in Champernowne's constant. */
public class Problem040 {
    public static void main(String[] args) {
        int product = 1;
        for (int position = 1; position <= 1_000_000; position *= 10) {
            product *= digitAt(position);
        }
        System.out.println(product);
    }

    private static int digitAt(int position) {
        int digits = 1;
        int blockCount = 9;
        int first = 1;
        while (position > digits * blockCount) {
            position -= digits * blockCount;
            digits++;
            blockCount *= 10;
            first *= 10;
        }
        int number = first + (position - 1) / digits;
        int index = (position - 1) % digits;
        return Integer.toString(number).charAt(index) - '0';
    }
}
