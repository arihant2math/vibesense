const std = @import("std");

// Project Euler 44: smallest difference of pentagonal numbers.
pub fn main() !void {
    var best: u64 = std.math.maxInt(u64);
    var j: u64 = 2;
    while (j <= 5_000) : (j += 1) {
        const pj = pentagonal(j);
        var k: u64 = 1;
        while (k < j) : (k += 1) {
            const pk = pentagonal(k);
            const difference = pj - pk;
            if (difference < best and isPentagonal(difference) and isPentagonal(pj + pk)) {
                best = difference;
            }
        }
    }
    printNumber(best);
}

fn pentagonal(n: u64) u64 {
    return n * (3 * n - 1) / 2;
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
