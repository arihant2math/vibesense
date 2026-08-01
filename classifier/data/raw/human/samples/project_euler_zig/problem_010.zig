const std = @import("std");

// Project Euler Problem 10: sum all primes below two million using a sieve.
pub fn main() !void {
    const limit = 2_000_000;
    var composite: [limit]bool = [_]bool{false} ** limit;

    var prime: usize = 2;
    while (prime <= (limit - 1) / prime) : (prime += 1) {
        if (composite[prime]) continue;
        var multiple = prime * prime;
        while (multiple < limit) : (multiple += prime) {
            composite[multiple] = true;
        }
    }

    var sum: u64 = 0;
    var n: usize = 2;
    while (n < limit) : (n += 1) {
        if (!composite[n]) sum += n;
    }
    try printNumber(sum);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
