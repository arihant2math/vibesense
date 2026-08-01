const std = @import("std");

// Project Euler 47: first four consecutive integers with four prime factors.
pub fn main() !void {
    var consecutive: u32 = 0;
    var n: u32 = 2;
    while (true) : (n += 1) {
        if (distinctFactorCount(n) == 4) {
            consecutive += 1;
            if (consecutive == 4) {
                printNumber(n - 3);
                return;
            }
        } else {
            consecutive = 0;
        }
    }
}

fn distinctFactorCount(number: u32) u32 {
    var n = number;
    var count: u32 = 0;
    var factor: u32 = 2;
    while (factor <= n / factor) : (factor += if (factor == 2) 1 else 2) {
        if (n % factor != 0) continue;
        count += 1;
        while (n % factor == 0) n /= factor;
    }
    if (n > 1) count += 1;
    return count;
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
