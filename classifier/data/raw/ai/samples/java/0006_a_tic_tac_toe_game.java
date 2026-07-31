import java.util.Scanner;

public class TicTacToe {
    private static final char EMPTY = ' ';
    private static final char HUMAN = 'X';
    private static final char COMPUTER = 'O';

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        char[] board = new char[9];

        for (int i = 0; i < board.length; i++) {
            board[i] = EMPTY;
        }

        System.out.println("Tic-Tac-Toe");
        System.out.println("You are X. Enter positions 1-9:");

        while (true) {
            printBoard(board);
            int move = readMove(scanner, board);
            board[move] = HUMAN;

            if (hasWon(board, HUMAN)) {
                printBoard(board);
                System.out.println("You win!");
                break;
            }

            if (isFull(board)) {
                printBoard(board);
                System.out.println("Draw!");
                break;
            }

            int computerMove = findBestMove(board);
            board[computerMove] = COMPUTER;
            System.out.println("Computer chose position " + (computerMove + 1) + ".");

            if (hasWon(board, COMPUTER)) {
                printBoard(board);
                System.out.println("Computer wins!");
                break;
            }

            if (isFull(board)) {
                printBoard(board);
                System.out.println("Draw!");
                break;
            }
        }

        scanner.close();
    }

    private static int readMove(Scanner scanner, char[] board) {
        while (true) {
            System.out.print("Your move: ");

            if (scanner.hasNextInt()) {
                int position = scanner.nextInt() - 1;

                if (position >= 0 && position < 9 && board[position] == EMPTY) {
                    return position;
                }
            } else {
                scanner.next();
            }

            System.out.println("Invalid move. Choose an empty position from 1 to 9.");
        }
    }

    private static int findBestMove(char[] board) {
        int bestScore = Integer.MIN_VALUE;
        int bestMove = -1;

        for (int i = 0; i < board.length; i++) {
            if (board[i] == EMPTY) {
                board[i] = COMPUTER;
                int score = minimax(board, false);
                board[i] = EMPTY;

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = i;
                }
            }
        }

        return bestMove;
    }

    private static int minimax(char[] board, boolean maximizing) {
        if (hasWon(board, COMPUTER)) {
            return 10;
        }

        if (hasWon(board, HUMAN)) {
            return -10;
        }

        if (isFull(board)) {
            return 0;
        }

        if (maximizing) {
            int bestScore = Integer.MIN_VALUE;

            for (int i = 0; i < board.length; i++) {
                if (board[i] == EMPTY) {
                    board[i] = COMPUTER;
                    bestScore = Math.max(bestScore, minimax(board, false));
                    board[i] = EMPTY;
                }
            }

            return bestScore;
        } else {
            int bestScore = Integer.MAX_VALUE;

            for (int i = 0; i < board.length; i++) {
                if (board[i] == EMPTY) {
                    board[i] = HUMAN;
                    bestScore = Math.min(bestScore, minimax(board, true));
                    board[i] = EMPTY;
                }
            }

            return bestScore;
        }
    }

    private static boolean hasWon(char[] board, char player) {
        int[][] wins = {
            {0, 1, 2},
            {3, 4, 5},
            {6, 7, 8},
            {0, 3, 6},
            {1, 4, 7},
            {2, 5, 8},
            {0, 4, 8},
            {2, 4, 6}
        };

        for (int[] win : wins) {
            if (board[win[0]] == player &&
                board[win[1]] == player &&
                board[win[2]] == player) {
                return true;
            }
        }

        return false;
    }

    private static boolean isFull(char[] board) {
        for (char cell : board) {
            if (cell == EMPTY) {
                return false;
            }
        }

        return true;
    }

    private static void printBoard(char[] board) {
        System.out.println();
        for (int row = 0; row < 3; row++) {
            int index = row * 3;

            System.out.printf(
                " %c | %c | %c%n",
                displayCell(board, index),
                displayCell(board, index + 1),
                displayCell(board, index + 2)
            );

            if (row < 2) {
                System.out.println("---+---+---");
            }
        }
        System.out.println();
    }

    private static char displayCell(char[] board, int index) {
        return board[index] == EMPTY ? (char) ('1' + index) : board[index];
    }
}
