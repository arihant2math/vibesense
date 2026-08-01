// Project Euler Problem 71: fraction immediately left of 3/7.
const std = @import("std");

pub fn main() !void {
    var best_num: u64 = 0;
    var best_den: u64 = 1;
    var den: u64 = 2;
    while (den <= 1_000_000) : (den += 1) {
        const num = (3 * den - 1) / 7;
        if (num * best_den > best_num * den) {
            best_num = num;
            best_den = den;
        }
    }
    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{best_num});
    try out.interface.flush();
}
