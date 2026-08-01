const std = @import("std");
// Project Euler 56: greatest digital sum of a^b for a,b below one hundred.
fn digitSum(a: u32, b: u32) u32 {
    var number: [256]u8 = [_]u8{0} ** 256;
    var used: usize = 1;
    number[0] = 1;
    for (0..b) |_| {
        var carry: u32 = 0;
        for (0..used) |i| {
            const v: u32 = number[i] * a + carry;
            number[i] = @intCast(v % 10);
            carry = v / 10;
        }
        while (carry != 0) : (carry /= 10) {
            number[used] = @intCast(carry % 10);
            used += 1;
        }
    }
    var sum: u32 = 0;
    for (number[0..used]) |d| sum += d;
    return sum;
}
pub fn main() !void {
    var best: u32 = 0;
    for (1..100) |a| {
        for (1..100) |b| {
            best = @max(best, digitSum(@intCast(a), @intCast(b)));
        }
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var out: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &out);
    try w.interface.print("{d}\n", .{best});
    try w.interface.flush();
}
