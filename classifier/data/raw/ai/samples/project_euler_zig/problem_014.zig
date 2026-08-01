// Project Euler 014: starting number below one million with the longest Collatz chain.
const std = @import("std");

fn print(answer: usize) !void {
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{answer});
    _ = std.c.write(1, text.ptr, text.len);
}

pub fn main() !void {
    var lengths = [_]u32{0} ** 1_000_001;
    lengths[1] = 1;
    var best_start: usize = 1;
    var best_length: u32 = 1;

    for (1..1_000_000) |start| {
        var value: u64 = start;
        var chain: [600]u64 = undefined;
        var used: usize = 0;
        while (value > 1 and (value > 1_000_000 or lengths[@intCast(value)] == 0)) {
            chain[used] = value;
            used += 1;
            value = if (value % 2 == 0) value / 2 else 3 * value + 1;
        }
        var length: u32 = if (value <= 1_000_000) lengths[@intCast(value)] else 1;
        while (used > 0) {
            used -= 1;
            length += 1;
            if (chain[used] <= 1_000_000) lengths[@intCast(chain[used])] = length;
        }
        if (lengths[start] > best_length) {
            best_length = lengths[start];
            best_start = start;
        }
    }
    try print(best_start);
}
