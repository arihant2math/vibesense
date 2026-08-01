const std = @import("std");

// Project Euler Problem 9: product of the Pythagorean triplet whose sum is 1000.
pub fn main() !void {
    var answer: u64 = 0;
    var a: u64 = 1;
    search: while (a < 1000) : (a += 1) {
        var b: u64 = a + 1;
        while (b < 1000 - a) : (b += 1) {
            const c = 1000 - a - b;
            if (a * a + b * b == c * c) {
                answer = a * b * c;
                break :search;
            }
        }
    }
    try printNumber(answer);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
