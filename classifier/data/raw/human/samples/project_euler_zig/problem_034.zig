const std = @import("std");

// Project Euler 34: sum numbers equal to the factorials of their digits.
pub fn main() !void {
    const factorial = [_]u32{ 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880 };
    var sum: u32 = 0;
    var n: u32 = 10;
    while (n <= 7 * factorial[9]) : (n += 1) {
        var remaining = n;
        var digit_sum: u32 = 0;
        while (remaining > 0) : (remaining /= 10) digit_sum += factorial[remaining % 10];
        if (digit_sum == n) sum += n;
    }
    try printAnswer(sum);
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
