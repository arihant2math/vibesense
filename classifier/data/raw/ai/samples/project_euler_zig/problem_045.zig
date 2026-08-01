const std = @import("std");

// Project Euler 45: next triangular, pentagonal, and hexagonal number.
pub fn main() !void {
    var n: u64 = 144; // H(143) = 40755 is the given previous value.
    while (true) : (n += 1) {
        const hexagonal = n * (2 * n - 1);
        if (isPentagonal(hexagonal)) {
            printNumber(hexagonal);
            return;
        }
    }
}

fn isPentagonal(n: u64) bool {
    const discriminant = 1 + 24 * n;
    var root: u64 = @intFromFloat(std.math.sqrt(@as(f64, @floatFromInt(discriminant))));
    while ((root + 1) <= discriminant / (root + 1)) : (root += 1) {}
    while (root > discriminant / root) : (root -= 1) {}
    return root * root == discriminant and (root + 1) % 6 == 0;
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
