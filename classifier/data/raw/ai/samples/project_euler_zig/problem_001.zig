const std = @import("std");

// Project Euler Problem 1: sum multiples of 3 or 5 below 1000.
pub fn main() !void {
    var sum: u64 = 0;
    var n: u64 = 0;
    while (n < 1000) : (n += 1) {
        if (n % 3 == 0 or n % 5 == 0) sum += n;
    }
    try printNumber(sum);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
