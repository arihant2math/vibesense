const std = @import("std");

// Project Euler 94: sum perimeters of almost-equilateral integral-area triangles.
pub fn main() !void {
    const limit: i64 = 1_000_000_000;
    var previous: i64 = 1; // Degenerate predecessor of (5, 5, 6).
    var equal_side: i64 = 5;
    var base_delta: i64 = 1; // The base is equal_side + base_delta.
    var sum: i64 = 0;
    while (true) {
        const perimeter = 3 * equal_side + base_delta;
        if (perimeter > limit) break;
        sum += perimeter;
        const next = 4 * equal_side - previous - 2 * base_delta;
        previous = equal_side;
        equal_side = next;
        base_delta = -base_delta;
    }
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{sum})));
}
