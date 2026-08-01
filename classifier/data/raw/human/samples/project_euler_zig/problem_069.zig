const std = @import("std");

// Project Euler 69: n <= one million maximizing n / phi(n).
fn isPrime(n: u64) bool {
    if (n < 2) return false;
    var divisor: u64 = 2;
    while (divisor * divisor <= n) : (divisor += 1) {
        if (n % divisor == 0) return false;
    }
    return true;
}

pub fn main() !void {
    const limit: u64 = 1_000_000;
    var product: u64 = 1;
    var candidate: u64 = 2;
    while (true) : (candidate += 1) {
        if (!isPrime(candidate)) continue;
        if (product * candidate > limit) break;
        product *= candidate;
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{product});
    _ = std.c.write(1, text.ptr, text.len);
}
