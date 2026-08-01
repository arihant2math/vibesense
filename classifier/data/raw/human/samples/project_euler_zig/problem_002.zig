const std = @import("std");

// Project Euler Problem 2: sum even Fibonacci terms not exceeding four million.
pub fn main() !void {
    var previous: u64 = 1;
    var current: u64 = 2;
    var sum: u64 = 0;

    while (current <= 4_000_000) {
        if (current % 2 == 0) sum += current;
        const next = previous + current;
        previous = current;
        current = next;
    }
    try printNumber(sum);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
