// Project Euler 79: derive the shortest passcode consistent with the keylog attempts.
public class Problem079 {
    private static final String[] ATTEMPTS = {
        "319", "680", "180", "690", "129", "620", "762", "689", "762", "318",
        "368", "710", "720", "710", "629", "168", "160", "689", "716", "731",
        "736", "729", "316", "729", "729", "710", "769", "290", "719", "680",
        "318", "389", "162", "289", "162", "718", "729", "319", "790", "680",
        "890", "362", "319", "760", "316", "729", "380", "319", "728", "716"
    };

    public static void main(String[] args) {
        boolean[][] before = new boolean[10][10];
        boolean[] present = new boolean[10];
        for (String attempt : ATTEMPTS) {
            int a = attempt.charAt(0) - '0';
            int b = attempt.charAt(1) - '0';
            int c = attempt.charAt(2) - '0';
            present[a] = present[b] = present[c] = true;
            before[a][b] = true;
            before[b][c] = true;
            before[a][c] = true;
        }

        StringBuilder passcode = new StringBuilder();
        for (int used = 0; used < 10; used++) {
            int next = -1;
            for (int digit = 0; digit < 10; digit++) {
                if (present[digit] && hasNoPredecessor(digit, before, present)) {
                    next = digit;
                    break;
                }
            }
            if (next == -1) {
                break;
            }
            passcode.append(next);
            present[next] = false;
        }
        System.out.println(passcode);
    }

    private static boolean hasNoPredecessor(int digit, boolean[][] before, boolean[] present) {
        for (int predecessor = 0; predecessor < 10; predecessor++) {
            if (present[predecessor] && before[predecessor][digit]) {
                return false;
            }
        }
        return true;
    }
}
