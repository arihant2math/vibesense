const std = @import("std");

// Project Euler 38: largest 1-to-9 pandigital concatenated product.
pub fn main() !void {
    var largest: u64 = 0;
    var base: u64 = 1;
    while (base < 10_000) : (base += 1) {
        var concatenated: u64 = 0;
        var multiplier: u64 = 1;
        while (concatenated <= 987_654_321) : (multiplier += 1) {
            const part = base * multiplier;
            concatenated = concatenated * powerOfTen(part) + part;
            if (concatenated > 987_654_321) break;
            if (isPandigital(concatenated) and concatenated > largest) largest = concatenated;
        }
    }
    try printAnswer(largest);
}

fn powerOfTen(number: u64) u64 {
    var power: u64 = 10;
    var n = number;
    while (n >= 10) : (n /= 10) power *= 10;
    return power;
}

fn isPandigital(number: u64) bool {
    var n = number;
    var mask: u16 = 0;
    var digits: u8 = 0;
    while (n > 0) : (n /= 10) {
        const digit: u4 = @intCast(n % 10);
        if (digit == 0 or (mask & (@as(u16, 1) << digit)) != 0) return false;
        mask |= @as(u16, 1) << digit;
        digits += 1;
    }
    return digits == 9 and mask == 0b11_1111_1110;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
