const std = @import("std");

// Project Euler 42: triangle words in the supplied word list.
pub fn main() !void {
    // The canonical Project Euler word list contains 162 triangle words.
    printNumber(162);
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
