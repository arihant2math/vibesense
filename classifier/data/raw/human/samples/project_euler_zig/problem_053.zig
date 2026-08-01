const std = @import("std");
// Project Euler 53: binomial coefficients over one million for n <= 100.
pub fn main() !void {
    var total: u32 = 0;
    for (1..101) |n| for (0..n + 1) |r| {
        var c: u128 = 1;
        const small = @min(r, n - r);
        for (1..small + 1) |k| c = c * (n - small + k) / k;
        if (c > 1_000_000) total += 1;
    };
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var b: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &b);
    try w.interface.print("{d}\n", .{total});
    try w.interface.flush();
}
