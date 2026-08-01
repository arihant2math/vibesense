// Project Euler 016: sum of the decimal digits of 2^1000.
const std = @import("std");

pub fn main() !void {
    var digits = [_]u8{0} ** 400;
    digits[0] = 1;
    var length: usize = 1;
    for (0..1000) |_| {
        var carry: u16 = 0;
        for (0..length) |i| {
            const value: u16 = @as(u16, digits[i]) * 2 + carry;
            digits[i] = @intCast(value % 10);
            carry = value / 10;
        }
        while (carry > 0) {
            digits[length] = @intCast(carry % 10);
            carry /= 10;
            length += 1;
        }
    }
    var sum: u32 = 0;
    for (digits[0..length]) |digit| sum += digit;
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{sum});
    _ = std.c.write(1, text.ptr, text.len);
}
