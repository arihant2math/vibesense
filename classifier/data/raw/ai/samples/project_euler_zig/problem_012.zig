// Project Euler 012: first triangular number with over 500 divisors.
const std = @import("std");

fn print(answer: u64) !void {
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{answer});
    _ = std.c.write(1, text.ptr, text.len);
}

fn divisorCount(n: u64) u64 {
    var remaining = n;
    var factor: u64 = 2;
    var count: u64 = 1;
    while (factor * factor <= remaining) : (factor += 1) {
        var exponent: u64 = 0;
        while (remaining % factor == 0) {
            remaining /= factor;
            exponent += 1;
        }
        if (exponent != 0) count *= exponent + 1;
    }
    if (remaining > 1) count *= 2;
    return count;
}

pub fn main() !void {
    var index: u64 = 1;
    var triangle: u64 = 1;
    while (divisorCount(triangle) <= 500) {
        index += 1;
        triangle += index;
    }
    try print(triangle);
}
