// Project Euler Problem 18: maximum total from top to bottom of the given triangle.
public class Problem018 {
    private static final String TRIANGLE = """
            75
            95 64
            17 47 82
            18 35 87 10
            20 04 82 47 65
            19 01 23 75 03 34
            88 02 77 73 07 63 67
            99 65 04 28 06 16 70 92
            41 41 26 56 83 40 80 70 33
            41 48 72 33 47 32 37 16 94 29
            53 71 44 65 25 43 91 52 97 51 14
            70 11 33 28 77 73 17 78 39 68 17 57
            91 71 52 38 17 14 91 43 58 50 27 29 48
            63 66 04 68 89 53 67 30 73 16 69 87 40 31
            04 62 98 27 23 09 70 98 73 93 38 53 60 04 23
            """;

    public static void main(String[] args) {
        String[] rows = TRIANGLE.trim().split("\\R");
        int[][] values = new int[rows.length][];
        for (int row = 0; row < rows.length; row++) {
            String[] entries = rows[row].trim().split("\\s+");
            values[row] = new int[entries.length];
            for (int column = 0; column < entries.length; column++) values[row][column] = Integer.parseInt(entries[column]);
        }
        for (int row = values.length - 2; row >= 0; row--) {
            for (int column = 0; column < values[row].length; column++) {
                values[row][column] += Math.max(values[row + 1][column], values[row + 1][column + 1]);
            }
        }
        System.out.println(values[0][0]);
    }
}
