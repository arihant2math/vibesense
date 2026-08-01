const std = @import("std");

// Project Euler 70: totient permutation below ten million with minimum n / phi(n).
const limit: usize = 10_000_000;
var phi: [limit + 1]u32 = undefined;

fn digitKey(value_in: u32) u64 {
    var value = value_in;
    var counts = [10]u64{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    while (value != 0) : (value /= 10) counts[value % 10] += 1;
    var key: u64 = 0;
    for (counts) |count| key = key * 16 + count;
    return key;
}

pub fn main() !void {
    for (0..limit + 1) |n| phi[n] = @intCast(n);
    var prime: usize = 2;
    while (prime <= limit) : (prime += 1) {
        if (phi[prime] != prime) continue;
        var multiple = prime;
        while (multiple <= limit) : (multiple += prime) {
            phi[multiple] -= phi[multiple] / @as(u32, @intCast(prime));
        }
    }
    var best_n: u32 = 0;
    var best_phi: u32 = 1;
    for (2..limit + 1) |n| {
        const n_u32: u32 = @intCast(n);
        if (digitKey(n_u32) != digitKey(phi[n])) continue;
        if (best_n == 0 or @as(u64, n_u32) * best_phi < @as(u64, best_n) * phi[n]) {
            best_n = n_u32;
            best_phi = phi[n];
        }
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{best_n});
    _ = std.c.write(1, text.ptr, text.len);
}
