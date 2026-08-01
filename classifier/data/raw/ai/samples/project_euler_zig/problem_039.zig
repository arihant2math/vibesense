const std = @import("std");

// Project Euler 39: perimeter at most 1000 with the most integer right triangles.
pub fn main() !void {
    var best_perimeter: u32 = 0;
    var best_count: u32 = 0;
    var perimeter: u32 = 12;
    while (perimeter <= 1000) : (perimeter += 1) {
        var count: u32 = 0;
        var a: u32 = 1;
        while (a < perimeter / 3) : (a += 1) {
            var b = a;
            while (b < (perimeter - a) / 2) : (b += 1) {
                const c = perimeter - a - b;
                if (a * a + b * b == c * c) count += 1;
            }
        }
        if (count > best_count) {
            best_count = count;
            best_perimeter = perimeter;
        }
    }
    try printAnswer(best_perimeter);
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
