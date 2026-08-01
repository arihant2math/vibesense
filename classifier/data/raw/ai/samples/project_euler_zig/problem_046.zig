const std = @import("std");

// Project Euler 46: smallest odd composite not prime plus twice a square.
pub fn main() !void {
    var n: u32 = 9;
    while (true) : (n += 2) {
        if (isPrime(n)) continue;
        if (!hasGoldbachForm(n)) {
            printNumber(n);
            return;
        }
    }
}

fn hasGoldbachForm(n: u32) bool {
    var prime: u32 = 2;
    while (prime < n) : (prime += 1) {
        if (!isPrime(prime)) continue;
        const remainder = n - prime;
        if (remainder % 2 != 0) continue;
        const half = remainder / 2;
        var square: u32 = 1;
        while (square <= half / square) : (square += 1) {
            if (square * square == half) return true;
        }
    }
    return false;
}

fn isPrime(n: u32) bool {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    var divisor: u32 = 3;
    while (divisor <= n / divisor) : (divisor += 2) {
        if (n % divisor == 0) return false;
    }
    return true;
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
