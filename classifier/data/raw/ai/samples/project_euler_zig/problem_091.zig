const std = @import("std");

// Project Euler 91: right triangles whose vertices lie in a 50 by 50 grid.
pub fn main() !void {
    const side: usize = 50;
    var count: u64 = 0;
    for (1..(side + 1) * (side + 1)) |first| {
        const ax: i64 = @intCast(first / (side + 1));
        const ay: i64 = @intCast(first % (side + 1));
        for ((first + 1)..(side + 1) * (side + 1)) |second| {
            const bx: i64 = @intCast(second / (side + 1));
            const by: i64 = @intCast(second % (side + 1));
            const dot = ax * bx + ay * by;
            if (dot == 0 or dot == ax * ax + ay * ay or dot == bx * bx + by * by)
                count += 1;
        }
    }
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{count})));
}
