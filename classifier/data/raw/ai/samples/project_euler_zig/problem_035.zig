const std = @import("std");

// Project Euler 35: count circular primes below one million.
pub fn main() !void {
    const limit = 1_000_000;
    var prime = [_]bool{true} ** limit;
    prime[0] = false;
    prime[1] = false;
    var p: usize = 2;
    while (p * p < limit) : (p += 1) {
        if (prime[p]) {
            var multiple = p * p;
            while (multiple < limit) : (multiple += p) prime[multiple] = false;
        }
    }

    var count: u32 = 0;
    var n: usize = 2;
    while (n < limit) : (n += 1) {
        if (prime[n] and isCircular(n, &prime)) count += 1;
    }
    try printAnswer(count);
}

fn isCircular(number: usize, prime: []const bool) bool {
    var divisor: usize = 1;
    while (divisor <= number / 10) : (divisor *= 10) {}
    var rotation = number;
    while (true) {
        if (!prime[rotation]) return false;
        rotation = (rotation % divisor) * 10 + rotation / divisor;
        if (rotation == number) return true;
    }
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
