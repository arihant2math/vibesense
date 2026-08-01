// Project Euler 96: solve the supplied fifty Sudoku puzzles and sum their top-left values.
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
public class Problem096 {
    private static final String DATA = "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";
    private static String input() throws IOException {
        try (InputStream in = new java.util.zip.GZIPInputStream(new ByteArrayInputStream(Base64.getDecoder().decode(DATA)))) {
            return new Scanner(in, "US-ASCII").useDelimiter("\\A").next();
        }
    }
    private static boolean solve(int[] board) {
        int cell = -1, choices = 0, fewest = 10;
        for (int i = 0; i < 81; i++) if (board[i] == 0) {
            int used = 0, r = i / 9, c = i % 9;
            for (int k = 0; k < 9; k++) { used |= 1 << board[r * 9 + k]; used |= 1 << board[k * 9 + c]; }
            int br = r / 3 * 3, bc = c / 3 * 3;
            for (int rr = br; rr < br + 3; rr++) for (int cc = bc; cc < bc + 3; cc++) used |= 1 << board[rr * 9 + cc];
            int options = (~used) & 0x3fe, count = Integer.bitCount(options);
            if (count < fewest) { fewest = count; cell = i; choices = options; if (count == 1) break; }
        }
        if (cell == -1) return true;
        while (choices != 0) {
            int bit = choices & -choices; choices -= bit; board[cell] = Integer.numberOfTrailingZeros(bit);
            if (solve(board)) return true;
        }
        board[cell] = 0; return false;
    }
    public static void main(String[] args) throws Exception {
        int total = 0, row = 0; int[] board = new int[81];
        for (String line : input().split("\\R")) if (line.matches("[0-9]{9}")) {
            for (int c = 0; c < 9; c++) board[row * 9 + c] = line.charAt(c) - '0';
            if (++row == 9) { solve(board); total += 100 * board[0] + 10 * board[1] + board[2]; row = 0; board = new int[81]; }
        }
        System.out.println(total);
    }
}
