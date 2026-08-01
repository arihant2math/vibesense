// Project Euler Problem 72: count reduced proper fractions with denominator at most one million.
const std = @import("std");

pub fn main() !void {
    const limit = 1_000_000;
    const phi = try std.heap.page_allocator.alloc(u64, limit + 1);
    defer std.heap.page_allocator.free(phi);

    for (0..limit + 1) |i| phi[i] = i;
    var p: usize = 2;
    while (p <= limit) : (p += 1) {
        if (phi[p] == p) {
            var multiple = p;
            while (multiple <= limit) : (multiple += p) {
                phi[multiple] -= phi[multiple] / p;
            }
        }
    }

    var total: u64 = 0;
    for (phi[2..]) |value| total += value;
    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{total});
    try out.interface.flush();
}
