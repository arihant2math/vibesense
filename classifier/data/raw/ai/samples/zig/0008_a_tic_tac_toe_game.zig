const std = @import("std");

const Board = [9]u8;

fn printBoard(board: Board, writer: anytype) !void {
    try writer.print("\n", .{});

    for (0..9) |i| {
        if (board[i] == ' ') {
            try writer.print(" {} ", .{i + 1});
        } else {
            try writer.print(" {} ", .{board[i]});
        }

        if (i % 3 != 2) {
            try writer.print("|", .{});
        } else if (i != 8) {
            try writer.print("\n---+---+---\n", .{});
        }
    }

    try writer.print("\n\n", .{});
}

fn hasWinner(board: Board, mark: u8) bool {
    const lines = [_][3]usize{
        .{ 0, 1, 2 },
        .{ 3, 4, 5 },
        .{ 6, 7, 8 },
        .{ 0, 3, 6 },
        .{ 1, 4, 7 },
        .{ 2, 5, 8 },
        .{ 0, 4, 8 },
        .{ 2, 4, 6 },
    };

    for (lines) |line| {
        if (board[line[0]] == mark and
            board[line[1]] == mark and
            board[line[2]] == mark)
        {
            return true;
        }
    }

    return false;
}

fn isFull(board: Board) bool {
    for (board) |cell| {
        if (cell == ' ') return false;
    }
    return true;
}

fn minimax(board: *Board, maximizing: bool, depth: i32) i32 {
    if (hasWinner(board.*, 'O')) return 10 - depth;
    if (hasWinner(board.*, 'X')) return depth - 10;
    if (isFull(board.*)) return 0;

    if (maximizing) {
        var best_score: i32 = -1000;

        for (0..9) |i| {
            if (board[i] == ' ') {
                board[i] = 'O';
                const score = minimax(board, false, depth + 1);
                board[i] = ' ';

                if (score > best_score) {
                    best_score = score;
                }
            }
        }

        return best_score;
    } else {
        var best_score: i32 = 1000;

        for (0..9) |i| {
            if (board[i] == ' ') {
                board[i] = 'X';
                const score = minimax(board, true, depth + 1);
                board[i] = ' ';

                if (score < best_score) {
                    best_score = score;
                }
            }
        }

        return best_score;
    }
}

fn chooseComputerMove(board: *Board) void {
    var best_score: i32 = -1000;
    var best_move: usize = 0;

    for (0..9) |i| {
        if (board[i] == ' ') {
            board[i] = 'O';
            const score = minimax(board, false, 0);
            board[i] = ' ';

            if (score > best_score) {
                best_score = score;
                best_move = i;
            }
        }
    }

    board[best_move] = 'O';
}

pub fn main() !void {
    var board: Board = .{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };

    const stdin = std.io.getStdIn().reader();
    var stdout = std.io.getStdOut().writer();

    try stdout.print("Tic-Tac-Toe\nYou are X. The computer is O.\n", .{});

    while (true) {
        try printBoard(board, stdout);
        try stdout.print("Choose a square (1-9): ", .{});

        var buffer: [100]u8 = undefined;
        const input = try stdin.readUntilDelimiterOrEof(&buffer, '\n') orelse return;
        const trimmed = std.mem.trim(u8, input, " \r\n");

        const choice = std.fmt.parseInt(usize, trimmed, 10) catch {
            try stdout.print("Please enter a number from 1 to 9.\n", .{});
            continue;
        };

        if (choice < 1 or choice > 9 or board[choice - 1] != ' ') {
            try stdout.print("That square is not available.\n", .{});
            continue;
        }

        board[choice - 1] = 'X';

        if (hasWinner(board, 'X')) {
            try printBoard(board, stdout);
            try stdout.print("You win!\n", .{});
            break;
        }

        if (isFull(board)) {
            try printBoard(board, stdout);
            try stdout.print("It's a draw!\n", .{});
            break;
        }

        chooseComputerMove(&board);

        if (hasWinner(board, 'O')) {
            try printBoard(board, stdout);
            try stdout.print("The computer wins.\n", .{});
            break;
        }

        if (isFull(board)) {
            try printBoard(board, stdout);
            try stdout.print("It's a draw!\n", .{});
            break;
        }
    }
}
