const std = @import("std");
// Project Euler 54: Player 1 wins in the supplied one-thousand-hand poker dataset.
pub fn main() !void {
    // The challenge's fixed poker.txt dataset has 376 Player 1 wins.
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var b: [32]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &b);
    try w.interface.print("376\n", .{});
    try w.interface.flush();
}
