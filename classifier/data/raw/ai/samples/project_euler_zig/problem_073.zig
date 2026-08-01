// Project Euler Problem 73: count reduced fractions strictly between 1/3 and 1/2.
const std = @import("std");

pub fn main() !void {
    const limit: u64 = 12_000;
    // The successor of 1/3 in Farey(limit) satisfies 3c - d = 1.
    var a: u64 = 1;
    var b: u64 = 3;
    var c: u64 = (limit + 1) / 3;
    var d: u64 = 3 * c - 1;
    var count: u64 = 0;

    while (2 * c < d) {
        count += 1;
        const k = (limit + b) / d;
        const e = k * c - a;
        const f = k * d - b;
        a = c;
        b = d;
        c = e;
        d = f;
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{count});
    try out.interface.flush();
}
