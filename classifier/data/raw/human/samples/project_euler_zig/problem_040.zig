const std = @import("std");

// Project Euler 40: product of selected digits in Champernowne's constant.
pub fn main() !void {
    const positions = [_]u32{ 1, 10, 100, 1_000, 10_000, 100_000, 1_000_000 };
    var product: u32 = 1;
    for (positions) |position| product *= digitAt(position);
    try printAnswer(product);
}

fn digitAt(position_initial: u32) u32 {
    var position = position_initial;
    var digits: u32 = 1;
    var count: u32 = 9;
    var first: u32 = 1;
    while (position > digits * count) {
        position -= digits * count;
        digits += 1;
        count *= 10;
        first *= 10;
    }
    const number = first + (position - 1) / digits;
    const index_from_left = (position - 1) % digits;
    var divisor: u32 = 1;
    var remaining = digits - index_from_left - 1;
    while (remaining > 0) : (remaining -= 1) divisor *= 10;
    return (number / divisor) % 10;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
