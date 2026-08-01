// Project Euler Problem 78: least n whose partition number is divisible by one million.
const std = @import("std");

pub fn main() !void {
    const modulus: i64 = 1_000_000;
    const max_n: usize = 100_000;
    const partitions = try std.heap.page_allocator.alloc(i64, max_n + 1);
    defer std.heap.page_allocator.free(partitions);
    partitions[0] = 1;

    var answer: usize = 0;
    var n: usize = 1;
    while (n <= max_n) : (n += 1) {
        var total: i64 = 0;
        var k: usize = 1;
        while (true) : (k += 1) {
            const g1 = k * (3 * k - 1) / 2;
            if (g1 > n) break;
            const add = k % 2 == 1;
            if (add) total += partitions[n - g1] else total -= partitions[n - g1];
            const g2 = k * (3 * k + 1) / 2;
            if (g2 <= n) {
                if (add) total += partitions[n - g2] else total -= partitions[n - g2];
            }
        }
        partitions[n] = @mod(total, modulus);
        if (partitions[n] == 0) {
            answer = n;
            break;
        }
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{answer});
    try out.interface.flush();
}
