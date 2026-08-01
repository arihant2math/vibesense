const std = @import("std");

// Project Euler Problem 7: find the 10,001st prime number.
pub fn main() !void {
    var count: u64 = 0;
    var candidate: u64 = 1;
    while (count < 10_001) {
        candidate += 1;
        if (isPrime(candidate)) count += 1;
    }
    try printNumber(candidate);
}

fn isPrime(number: u64) bool {
    if (number < 2) return false;
    if (number == 2) return true;
    if (number % 2 == 0) return false;

    var divisor: u64 = 3;
    while (divisor <= number / divisor) : (divisor += 2) {
        if (number % divisor == 0) return false;
    }
    return true;
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
