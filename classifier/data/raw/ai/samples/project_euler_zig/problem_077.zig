// Project Euler Problem 77: first integer expressible as a sum of primes in over 5000 ways.
const std = @import("std");

pub fn main() !void {
    const max = 1_000;
    var composite = [_]bool{false} ** (max + 1);
    var p: usize = 2;
    while (p * p <= max) : (p += 1) {
        if (!composite[p]) {
            var multiple = p * p;
            while (multiple <= max) : (multiple += p) composite[multiple] = true;
        }
    }

    var answer: usize = 0;
    var target: usize = 2;
    while (target <= max) : (target += 1) {
        var ways = [_]u32{0} ** (max + 1);
        ways[0] = 1;
        var prime: usize = 2;
        while (prime <= target) : (prime += 1) {
            if (composite[prime]) continue;
            var sum = prime;
            while (sum <= target) : (sum += 1) ways[sum] += ways[sum - prime];
        }
        if (ways[target] > 5_000) {
            answer = target;
            break;
        }
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{answer});
    try out.interface.flush();
}
