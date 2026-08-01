const std = @import("std");
// Project Euler 52: smallest integer whose first six multiples are permutations.
fn signature(n0: u64) u64 {
    var n = n0;
    var result: u64 = 0;
    if (n == 0) return 1;
    while (n != 0) : (n /= 10) result += @as(u64, 1) << @intCast((n % 10) * 4);
    return result;
}
pub fn main() !void {
    var n: u64 = 1;
    while (true) : (n += 1) {
        const s = signature(n);
        var k: u64 = 2;
        while (k <= 6 and signature(k * n) == s) : (k += 1) {}
        if (k == 7) break;
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var b: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &b);
    try w.interface.print("{d}\n", .{n});
    try w.interface.flush();
}
