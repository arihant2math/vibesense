const std = @import("std");

// Project Euler 49: concatenated prime-permutation arithmetic sequence.
pub fn main() !void {
    const limit = 10_000;
    var prime = [_]bool{true} ** limit;
    prime[0] = false;
    prime[1] = false;
    var p: usize = 2;
    while (p * p < limit) : (p += 1) {
        if (!prime[p]) continue;
        var multiple = p * p;
        while (multiple < limit) : (multiple += p) prime[multiple] = false;
    }

    var first: usize = 1000;
    while (first < limit) : (first += 1) {
        if (!prime[first]) continue;
        var second = first + 1;
        while (second < limit) : (second += 1) {
            const third = second + (second - first);
            if (third >= limit) break;
            if (!prime[second] or !prime[third]) continue;
            if (first == 1487 and second == 4817) continue;
            if (sameSignature(first, second) and sameSignature(first, third)) {
                const answer: u64 = @as(u64, first) * 100_000_000 + @as(u64, second) * 10_000 + third;
                printNumber(answer);
                return;
            }
        }
    }
}

fn sameSignature(first: usize, second: usize) bool {
    const first_counts = signature(first);
    const second_counts = signature(second);
    return std.mem.eql(u8, &first_counts, &second_counts);
}

fn signature(number: usize) [10]u8 {
    var counts = [_]u8{0} ** 10;
    var n = number;
    while (n != 0) : (n /= 10) counts[n % 10] += 1;
    return counts;
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
