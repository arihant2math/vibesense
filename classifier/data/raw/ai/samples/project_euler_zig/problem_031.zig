const std = @import("std");

// Project Euler 31: number of ways to make two pounds from British coins.
pub fn main() !void {
    const coins = [_]usize{ 1, 2, 5, 10, 20, 50, 100, 200 };
    var ways = [_]u64{0} ** 201;
    ways[0] = 1;
    for (coins) |coin| {
        var amount = coin;
        while (amount <= 200) : (amount += 1) ways[amount] += ways[amount - coin];
    }
    try printAnswer(ways[200]);
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
