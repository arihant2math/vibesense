const std = @import("std");

// Project Euler Problem 5: smallest positive number divisible by every integer from 1 to 20.
pub fn main() !void {
    var result: u64 = 1;
    var n: u64 = 2;
    while (n <= 20) : (n += 1) {
        result = result / gcd(result, n) * n;
    }
    try printNumber(result);
}

fn gcd(a: u64, b: u64) u64 {
    var left = a;
    var right = b;
    while (right != 0) {
        const remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
