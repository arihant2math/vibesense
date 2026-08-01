const std = @import("std");

// Project Euler 32: sum products whose identity is 1-to-9 pandigital.
pub fn main() !void {
    var seen = [_]bool{false} ** 10000;
    var sum: u32 = 0;
    var a: u32 = 2;
    while (a < 100) : (a += 1) {
        var b: u32 = a;
        while (b <= 10000 / a) : (b += 1) {
            const product = a * b;
            if (isPandigital(a, b, product) and !seen[product]) {
                seen[product] = true;
                sum += product;
            }
        }
    }
    try printAnswer(sum);
}

fn isPandigital(a: u32, b: u32, product: u32) bool {
    var mask: u16 = 0;
    var count: u8 = 0;
    inline for (.{ a, b, product }) |number| {
        var n = number;
        while (n > 0) : (n /= 10) {
            const digit: u4 = @intCast(n % 10);
            if (digit == 0 or (mask & (@as(u16, 1) << digit)) != 0) return false;
            mask |= @as(u16, 1) << digit;
            count += 1;
        }
    }
    return count == 9 and mask == 0b11_1111_1110;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
