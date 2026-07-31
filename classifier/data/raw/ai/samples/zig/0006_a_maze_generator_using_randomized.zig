const std = @import("std");

const AppError = error{
    MissingDimensions,
    TooManyArguments,
    InvalidRows,
    InvalidColumns,
    InvalidSeed,
    DimensionsTooLarge,
    AllocationTooLarge,
};

const Cell = struct {
    walls: u8 = 0b1111,
    visited: bool = false,
};

const Candidate = struct {
    index: usize,
    direction: u8,
};

const max_dimension: usize = 1000;
const max_cells: usize = 1_000_000;

fn parseDimension(value: []const u8, invalid_error: anyerror) !usize {
    if (value.len == 0) return invalid_error;

    const parsed = std.fmt.parseInt(usize, value, 10) catch return invalid_error;
    if (parsed == 0 or parsed > max_dimension) return invalid_error;

    return parsed;
}

fn parseSeed(value: []const u8) !u64 {
    if (value.len == 0) return AppError.InvalidSeed;
    return std.fmt.parseInt(u64, value, 10) catch AppError.InvalidSeed;
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    var args = std.process.args();
    _ = args.next();

    const rows_arg = args.next() orelse return AppError.MissingDimensions;
    const cols_arg = args.next() orelse return AppError.MissingDimensions;
    const seed_arg = args.next();

    if (args.next() != null) return AppError.TooManyArguments;

    const rows = try parseDimension(rows_arg, AppError.InvalidRows);
    const cols = try parseDimension(cols_arg, AppError.InvalidColumns);

    if (rows > max_cells / cols) return AppError.DimensionsTooLarge;

    const cell_count = rows * cols;
    const cells = allocator.alloc(Cell, cell_count) catch return AppError.AllocationTooLarge;
    defer allocator.free(cells);

    for (cells) |*cell| {
        cell.* = Cell{};
    }

    const seed: u64 = if (seed_arg) |value|
        try parseSeed(value)
    else
        0;

    var prng = std.Random.DefaultPrng.init(seed);
    const random = prng.random();

    var stack = std.ArrayList(usize).init(allocator);
    defer stack.deinit();

    cells[0].visited = true;
    try stack.append(0);

    while (stack.items.len > 0) {
        const current = stack.items[stack.items.len - 1];
        const row = current / cols;
        const col = current % cols;

        var candidates: [4]Candidate = undefined;
        var candidate_count: usize = 0;

        if (row > 0) {
            const index = current - cols;
            if (!cells[index].visited) {
                candidates[candidate_count] = .{ .index = index, .direction = 1 };
                candidate_count += 1;
            }
        }

        if (col + 1 < cols) {
            const index = current + 1;
            if (!cells[index].visited) {
                candidates[candidate_count] = .{ .index = index, .direction = 2 };
                candidate_count += 1;
            }
        }

        if (row + 1 < rows) {
            const index = current + cols;
            if (!cells[index].visited) {
                candidates[candidate_count] = .{ .index = index, .direction = 4 };
                candidate_count += 1;
            }
        }

        if (col > 0) {
            const index = current - 1;
            if (!cells[index].visited) {
                candidates[candidate_count] = .{ .index = index, .direction = 8 };
                candidate_count += 1;
            }
        }

        if (candidate_count == 0) {
            _ = stack.pop();
            continue;
        }

        const selected = candidates[random.uintLessThan(usize, candidate_count)];
        const opposite: u8 = switch (selected.direction) {
            1 => 4,
            2 => 8,
            4 => 1,
            8 => 2,
            else => unreachable,
        };

        cells[current].walls &= ~selected.direction;
        cells[selected.index].walls &= ~opposite;
        cells[selected.index].visited = true;

        try stack.append(selected.index);
    }

    var stdout = std.io.getStdOut().writer();

    try stdout.writeByte('+');
    for (0..cols) |_| {
        try stdout.writeAll("---+");
    }
    try stdout.writeByte('\n');

    for (0..rows) |row| {
        try stdout.writeByte('|');

        for (0..cols) |col| {
            const cell = cells[row * cols + col];

            try stdout.writeAll("   ");
            if ((cell.walls & 2) != 0) {
                try stdout.writeByte('|');
            } else {
                try stdout.writeByte(' ');
            }
        }
        try stdout.writeByte('\n');

        try stdout.writeByte('+');

        for (0..cols) |col| {
            const cell = cells[row * cols + col];

            if ((cell.walls & 4) != 0) {
                try stdout.writeAll("---");
            } else {
                try stdout.writeAll("   ");
            }

            try stdout.writeByte('+');
        }
        try stdout.writeByte('\n');
    }
}
