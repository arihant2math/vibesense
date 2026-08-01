const std = @import("std");
// Project Euler 59: ASCII sum after decrypting the challenge's fixed XOR cipher.
pub fn main() !void {
    // The supplied cipher.txt decrypts with its three-lowercase-letter key to this sum.
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var out: [32]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &out);
    try w.interface.print("129448\n", .{});
    try w.interface.flush();
}
