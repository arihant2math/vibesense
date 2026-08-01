const std = @import("std");

// Project Euler 86: least cuboid side length with over one million integer paths.
fn isSquare(value: u64) bool {
    const root: u64 = @intFromFloat(@sqrt(@as(f64, @floatFromInt(value))));
    return root * root == value or (root + 1) * (root + 1) == value;
}

pub fn main() !void {
    var count: u64 = 0;
    var largest_side: u64 = 0;

    while (count <= 1_000_000) {
        largest_side += 1;
        var sum_of_other_sides: u64 = 2;
        while (sum_of_other_sides <= 2 * largest_side) : (sum_of_other_sides += 1) {
            if (!isSquare(largest_side * largest_side + sum_of_other_sides * sum_of_other_sides)) continue;

            const low = if (sum_of_other_sides > largest_side) sum_of_other_sides - largest_side else 1;
            const high = @min(largest_side, sum_of_other_sides / 2);
            if (high >= low) count += high - low + 1;
        }
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d}\n", .{largest_side});
    try writer.interface.flush();
}
