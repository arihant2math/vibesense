const std = @import("std");

// Project Euler 87: sums below fifty million of prime squares, cubes, and fourth powers.
const limit: usize = 50_000_000;
const prime_limit: usize = 7_072;
var representable: [limit]bool = [_]bool{false} ** limit;

pub fn main() !void {
    var composite: [prime_limit]bool = [_]bool{false} ** prime_limit;
    var primes: [1_000]usize = undefined;
    var prime_count: usize = 0;

    var candidate: usize = 2;
    while (candidate < prime_limit) : (candidate += 1) {
        if (composite[candidate]) continue;
        primes[prime_count] = candidate;
        prime_count += 1;
        if (candidate * candidate < prime_limit) {
            var multiple = candidate * candidate;
            while (multiple < prime_limit) : (multiple += candidate) composite[multiple] = true;
        }
    }

    var i: usize = 0;
    while (i < prime_count) : (i += 1) {
        const square = primes[i] * primes[i];
        if (square >= limit) break;
        var j: usize = 0;
        while (j < prime_count) : (j += 1) {
            const cube = primes[j] * primes[j] * primes[j];
            if (square + cube >= limit) break;
            var k: usize = 0;
            while (k < prime_count) : (k += 1) {
                const p = primes[k];
                const fourth = p * p * p * p;
                const total = square + cube + fourth;
                if (total >= limit) break;
                representable[total] = true;
            }
        }
    }

    var count: usize = 0;
    for (&representable) |present| {
        if (present) count += 1;
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d}\n", .{count});
    try writer.interface.flush();
}
