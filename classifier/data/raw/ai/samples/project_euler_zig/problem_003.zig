const std = @import("std");

// Project Euler Problem 3: find the largest prime factor of 600851475143.
pub fn main() !void {
    var number: u64 = 600_851_475_143;
    var factor: u64 = 2;
    var largest: u64 = 1;

    while (factor <= number / factor) {
        while (number % factor == 0) {
            largest = factor;
            number /= factor;
        }
        factor += if (factor == 2) 1 else 2;
    }
    if (number > 1) largest = number;
    try printNumber(largest);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
