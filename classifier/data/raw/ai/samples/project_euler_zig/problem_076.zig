// Project Euler Problem 76: partitions of 100 into at least two positive integers.
const std = @import("std");

pub fn main() !void {
    var ways = [_]u64{0} ** 101;
    ways[0] = 1;
    for (1..100) |part| {
        var sum = part;
        while (sum <= 100) : (sum += 1) ways[sum] += ways[sum - part];
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{ways[100]});
    try out.interface.flush();
}
