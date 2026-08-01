const std = @import("std");

// Project Euler 33: denominator of the product of digit-cancelling fractions.
pub fn main() !void {
    var numerator_product: u32 = 1;
    var denominator_product: u32 = 1;
    var numerator: u32 = 10;
    while (numerator < 100) : (numerator += 1) {
        var denominator = numerator + 1;
        while (denominator < 100) : (denominator += 1) {
            if (numerator % 10 != 0 and numerator % 10 == denominator / 10 and
                (numerator / 10) * denominator == numerator * (denominator % 10))
            {
                numerator_product *= numerator;
                denominator_product *= denominator;
            }
        }
    }
    try printAnswer(denominator_product / gcd(numerator_product, denominator_product));
}

fn gcd(a_initial: u32, b_initial: u32) u32 {
    var a = a_initial;
    var b = b_initial;
    while (b != 0) {
        const remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
