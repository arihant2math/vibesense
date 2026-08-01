// Project Euler 015: number of lattice paths through a 20 by 20 grid.
const std = @import("std");

pub fn main() !void {
    var paths: u64 = 1;
    for (1..21) |i| paths = paths * (20 + i) / i;
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{paths});
    _ = std.c.write(1, text.ptr, text.len);
}
