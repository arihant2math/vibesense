const std = @import("std");

// Project Euler 63: n-digit positive integers that are also nth powers.
fn digits(value_in: u128) u32 {
    var value = value_in;
    var result: u32 = 0;
    while (value != 0) : (value /= 10) result += 1;
    return result;
}

pub fn main() !void {
    var count: u32 = 0;
    var exponent: u32 = 1;
    var nine_power: u128 = 1;
    while (true) : (exponent += 1) {
        nine_power *= 9;
        if (digits(nine_power) < exponent) break;
        for (1..10) |base| {
            var power: u128 = 1;
            var times: u32 = 0;
            while (times < exponent) : (times += 1) power *= base;
            if (digits(power) == exponent) count += 1;
        }
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{count});
    _ = std.c.write(1, text.ptr, text.len);
}
