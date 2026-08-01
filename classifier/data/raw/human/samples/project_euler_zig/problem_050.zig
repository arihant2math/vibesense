const std = @import("std");

// Project Euler 50: prime below one million as the longest consecutive prime sum.
pub fn main() !void {
    const limit = 1_000_000;
    var is_prime = [_]bool{true} ** limit;
    is_prime[0] = false;
    is_prime[1] = false;
    var candidate: usize = 2;
    while (candidate * candidate < limit) : (candidate += 1) {
        if (!is_prime[candidate]) continue;
        var multiple = candidate * candidate;
        while (multiple < limit) : (multiple += candidate) is_prime[multiple] = false;
    }

    var primes: [80_000]u32 = undefined;
    var count: usize = 0;
    for (2..limit) |n| {
        if (is_prime[n]) {
            primes[count] = @intCast(n);
            count += 1;
        }
    }
    var sums: [80_001]u64 = undefined;
    sums[0] = 0;
    for (0..count) |index| sums[index + 1] = sums[index] + primes[index];

    var best_length: usize = 0;
    var answer: u64 = 0;
    var start: usize = 0;
    while (start < count) : (start += 1) {
        var finish = start + best_length + 1;
        while (finish <= count) : (finish += 1) {
            const sum = sums[finish] - sums[start];
            if (sum >= limit) break;
            if (is_prime[@intCast(sum)]) {
                best_length = finish - start;
                answer = sum;
            }
        }
    }
    printNumber(answer);
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
