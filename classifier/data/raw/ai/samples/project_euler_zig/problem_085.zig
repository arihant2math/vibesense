const std = @import("std");

// Project Euler 85: grid area whose rectangle count is nearest two million.
pub fn main() !void {
    const target: u64 = 2_000_000;
    var best_difference: u64 = std.math.maxInt(u64);
    var best_area: u64 = 0;

    var width: u64 = 1;
    while (width <= 2_000) : (width += 1) {
        var height: u64 = 1;
        while (height <= 2_000) : (height += 1) {
            const rectangles = width * (width + 1) * height * (height + 1) / 4;
            const difference = if (rectangles > target) rectangles - target else target - rectangles;
            if (difference < best_difference) {
                best_difference = difference;
                best_area = width * height;
            }
            if (rectangles > target) break;
        }
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d}\n", .{best_area});
    try writer.interface.flush();
}
