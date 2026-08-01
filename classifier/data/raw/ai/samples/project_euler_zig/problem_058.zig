const std = @import("std");
// Project Euler 58: side length when spiral-diagonal prime ratio first falls below ten percent.
fn isPrime(n: u64) bool {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    var d: u64 = 3;
    while (d <= n / d) : (d += 2) if (n % d == 0) return false;
    return true;
}
pub fn main() !void {
    var side: u64 = 1;
    var corner: u64 = 1;
    var primes: u64 = 0;
    var diagonal: u64 = 1;
    while (true) {
        side += 2;
        const step = side - 1;
        for (0..3) |_| {
            corner += step;
            if (isPrime(corner)) primes += 1;
        }
        corner += step;
        diagonal += 4;
        if (primes * 10 < diagonal) break;
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var out: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &out);
    try w.interface.print("{d}\n", .{side});
    try w.interface.flush();
}
