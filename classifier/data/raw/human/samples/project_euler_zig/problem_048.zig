const std = @import("std");

// Project Euler 48: last ten digits of the self powers series.
pub fn main() !void {
    const modulus: u64 = 10_000_000_000;
    var total: u64 = 0;
    var n: u64 = 1;
    while (n <= 1000) : (n += 1) {
        var power: u64 = 1;
        var exponent: u64 = 0;
        while (exponent < n) : (exponent += 1) {
            power = @intCast((@as(u128, power) * n) % modulus);
        }
        total = @intCast((@as(u128, total) + power) % modulus);
    }
    printNumber(total);
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
