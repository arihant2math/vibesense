const std = @import("std");

// Project Euler 37: sum the eleven primes truncatable from both directions.
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

    var found: u8 = 0;
    var sum: u32 = 0;
    var n: usize = 11;
    while (n < limit and found < 11) : (n += 1) {
        if (prime[n] and isTruncatable(n, &prime)) {
            found += 1;
            sum += @intCast(n);
        }
    }
    try printAnswer(sum);
}

fn isTruncatable(number: usize, prime: []const bool) bool {
    var right = number;
    while (right >= 10) {
        right /= 10;
        if (!prime[right]) return false;
    }

    var divisor: usize = 1;
    while (divisor <= number / 10) : (divisor *= 10) {}
    var left = number;
    while (divisor > 1) {
        left %= divisor;
        if (!prime[left]) return false;
        divisor /= 10;
    }
    return true;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
