const std = @import("std");

// Project Euler 100: first blue-disc count above one trillion total discs.
pub fn main() !void {
    var blue: u64 = 15;
    var total: u64 = 21;
    while (total <= 1_000_000_000_000) {
        const next_blue = 3 * blue + 2 * total - 2;
        total = 4 * blue + 3 * total - 3;
        blue = next_blue;
    }
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{blue})));
}
