const std = @import("std");

const Board = [9]u8;
const Empty: u8 = 0;
const Human: u8 = 1;
const Computer: u8 = 2;

const winning_lines = [_][3]usize{
    .{ 0, 1, 2 },
    .{ 3, 4, 5 },
    .{ 6, 7, 8 },
    .{ 0, 3, 6 },
    .{ 1, 4, 7 },
    .{ 2, 5, 8 },
    .{ 0, 4, 8 },
    .{ 2, 4, 6 },
};

fn winner(board: *const Board) u8 {
    for (winning_lines) |line| {
        const a = line[0];
        const b = line[1];
        const c = line[2];

        if (board[a] != Empty and board[a] == board[b] and board[a] == board[c]) {
            return board[a];
        }
    }
    return Empty;
}

fn isFull(board: *const Board) bool {
    for (board) |cell| {
        if (cell == Empty) return false;
    }
    return true;
}

fn minimax(board: *Board, maximizing: bool) i8 {
    const result = winner(board);
    if (result == Computer) return 10;
    if (result == Human) return -10;
    if (isFull(board)) return 0;

    if (maximizing) {
        var best: i8 = -10;

        for (0..board.len) |i| {
            if (board[i] == Empty) {
                board[i] = Computer;
                const score = minimax(board, false);
                board[i] = Empty;

                if (score > best) best = score;
            }
        }
        return best;
    } else {
        var best: i8 = 10;

        for (0..board.len) |i| {
            if (board[i] == Empty) {
                board[i] = Human;
                const score = minimax(board, true);
                board[i] = Empty;

                if (score < best) best = score;
            }
        }
        return best;
    }
}

fn bestMove(board: *Board) ?usize {
    var best_score: i8 = -10;
    var move: ?usize = null;

    for (0..board.len) |i| {
        if (board[i] == Empty) {
            board[i] = Computer;
            const score = minimax(board, false);
            board[i] = Empty;

            if (score > best_score) {
                best_score = score;
                move = i;
            }
        }
    }

    return move;
}

fn printBoard(writer: anytype, board: *const Board) !void {
    for (0..board.len) |i| {
        if (board[i] == Human) {
            try writer.print(" X ", .{});
        } else if (board[i] == Computer) {
            try writer.print(" O ", .{});
        } else {
            try writer.print(" {d} ", .{i + 1});
        }

        if (i % 3 == 2) {
            try writer.writeByte('\n');
        } else {
            try writer.writeAll("|");
        }
    }
}

pub fn main() !void {
    const stdin = std.io.getStdIn().reader();
    const stdout = std.io.getStdOut().writer();

    var input_buffer: [64]u8 = undefined;
    var board: Board = [_]u8{Empty} ** 9;

    try stdout.writeAll("Tic-Tac-Toe: you are X, computer is O.\n");

    while (true) {
        try printBoard(stdout, &board);

        const result = winner(&board);
        if (result == Human) {
            try stdout.writeAll("You win!\n");
            break;
        } else if (result == Computer) {
            try stdout.writeAll("Computer wins!\n");
            break;
        } else if (isFull(&board)) {
            try stdout.writeAll("Draw!\n");
            break;
        }

        try stdout.writeAll("Choose a square (1-9): ");
        const line = (try stdin.readUntilDelimiterOrEof(&input_buffer, '\n')) orelse break;
        const text = std.mem.trim(u8, line, " \t\r\n");

        const parsed = std.fmt.parseInt(u8, text, 10) catch {
            try stdout.writeAll("Invalid input.\n");
            continue;
        };

        if (parsed < 1 or parsed > 9) {
            try stdout.writeAll("Choose a number from 1 to 9.\n");
            continue;
        }

        const index: usize = @as(usize, parsed) - 1;
        if (board[index] != Empty) {
            try stdout.writeAll("That square is occupied.\n");
            continue;
        }

        board[index] = Human;

        if (winner(&board) == Human or isFull(&board)) continue;

        if (bestMove(&board)) |computer_index| {
            board[computer_index] = Computer;
        }
    }
}
