const std = @import("std");

// Project Euler 51: smallest prime in an eight-prime digit-replacement family.
const limit = 1_000_000;
var composite: [limit]bool = undefined;

fn isPrime(n: usize) bool {
    return n >= 2 and !composite[n];
}
fn digits(n: usize) usize {
    var x = n;
    var result: usize = 0;
    while (x != 0) : (x /= 10) result += 1;
    return result;
}
fn pow10(p: usize) usize {
    var result: usize = 1;
    for (0..p) |_| result *= 10;
    return result;
}

pub fn main() !void {
    @memset(&composite, false);
    composite[0] = true;
    composite[1] = true;
    for (2..limit) |p| {
        if (!composite[p] and p <= (limit - 1) / p) {
            var m = p * p;
            while (m < limit) : (m += p) composite[m] = true;
        }
    }

    var answer: usize = 0;
    outer: for (11..limit) |n| {
        if (!isPrime(n)) continue;
        const width = digits(n);
        for (0..10) |wanted| {
            var places: [6]usize = undefined;
            var count: usize = 0;
            // Do not replace the units digit: an eight-member family cannot need it.
            for (1..width) |pos| {
                if ((n / pow10(pos)) % 10 == wanted) {
                    places[count] = pos;
                    count += 1;
                }
            }
            for (1..(@as(usize, 1) << @intCast(count))) |mask| {
                var family: usize = 0;
                var replaces_leading = false;
                for (0..count) |j| {
                    if ((mask & (@as(usize, 1) << @intCast(j))) != 0 and places[j] == width - 1) replaces_leading = true;
                }
                for (0..10) |replacement| {
                    if (replacement == 0 and replaces_leading) continue;
                    var candidate: isize = @intCast(n);
                    for (0..count) |j| if ((mask & (@as(usize, 1) << @intCast(j))) != 0) {
                        candidate += (@as(isize, @intCast(replacement)) - @as(isize, @intCast(wanted))) * @as(isize, @intCast(pow10(places[j])));
                    };
                    if (candidate > 0 and @as(usize, @intCast(candidate)) < limit and isPrime(@intCast(candidate))) family += 1;
                }
                if (family >= 8) {
                    answer = n;
                    break :outer;
                }
            }
        }
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(io, &buffer);
    try writer.interface.print("{d}\n", .{answer});
    try writer.interface.flush();
}
