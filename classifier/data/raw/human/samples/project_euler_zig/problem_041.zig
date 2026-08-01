const std = @import("std");

// Project Euler 41: largest pandigital prime.
pub fn main() !void {
    var n: u32 = 7_654_321;
    while (n >= 1) : (n -= 2) {
        if (isPandigital7(n) and isPrime(n)) {
            printNumber(n);
            return;
        }
    }
}

fn isPandigital7(n: u32) bool {
    var value = n;
    var mask: u8 = 0;
    var count: u8 = 0;
    while (value != 0) : (value /= 10) {
        const digit: u8 = @intCast(value % 10);
        if (digit == 0 or digit > 7) return false;
        const bit: u8 = @as(u8, 1) << @intCast(digit - 1);
        if ((mask & bit) != 0) return false;
        mask |= bit;
        count += 1;
    }
    return count == 7 and mask == 0b111_1111;
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
