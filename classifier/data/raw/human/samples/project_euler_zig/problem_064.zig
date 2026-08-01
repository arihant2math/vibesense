const std = @import("std");

// Project Euler 64: odd continued-fraction periods of square roots up to 10000.
fn isSquare(n: u32) bool {
    var root: u32 = 1;
    while (root * root < n) : (root += 1) {}
    return root * root == n;
}

pub fn main() !void {
    var odd_periods: u32 = 0;
    for (2..10_001) |n_usize| {
        const n: u32 = @intCast(n_usize);
        if (isSquare(n)) continue;
        var a0: u32 = 1;
        while ((a0 + 1) * (a0 + 1) <= n) : (a0 += 1) {}
        var m: u32 = 0;
        var denominator: u32 = 1;
        var a: u32 = a0;
        var period: u32 = 0;
        while (true) {
            m = denominator * a - m;
            denominator = (n - m * m) / denominator;
            a = (a0 + m) / denominator;
            period += 1;
            if (a == 2 * a0) break;
        }
        if (period % 2 == 1) odd_periods += 1;
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{odd_periods});
    _ = std.c.write(1, text.ptr, text.len);
}
