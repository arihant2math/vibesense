const std = @import("std");

// Project Euler 88: sum of distinct minimal product-sum numbers for 2 <= k <= 12000.
const max_k: u32 = 12_000;
const max_product: u32 = 2 * max_k;
var minimum: [max_k + 1]u32 = [_]u32{max_product} ** (max_k + 1);

fn search(start: u32, product: u32, sum: u32, factors: u32) void {
    const k = product - sum + factors;
    if (k <= max_k and product < minimum[k]) minimum[k] = product;

    var factor = start;
    while (product * factor <= max_product) : (factor += 1) {
        search(factor, product * factor, sum + factor, factors + 1);
    }
}

pub fn main() !void {
    search(2, 1, 0, 0);

    var used: [max_product + 1]bool = [_]bool{false} ** (max_product + 1);
    var k: usize = 2;
    while (k <= max_k) : (k += 1) used[minimum[k]] = true;

    var answer: u64 = 0;
    for (used, 0..) |present, value| {
        if (present) answer += value;
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d}\n", .{answer});
    try writer.interface.flush();
}
