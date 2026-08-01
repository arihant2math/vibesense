const std = @import("std");

// Project Euler 36: sum numbers palindromic in both base ten and base two.
pub fn main() !void {
    var sum: u32 = 0;
    var n: u32 = 1;
    while (n < 1_000_000) : (n += 1) {
        if (isPalindrome(n, 10) and isPalindrome(n, 2)) sum += n;
    }
    try printAnswer(sum);
}

fn isPalindrome(number: u32, base: u32) bool {
    var n = number;
    var reversed: u32 = 0;
    while (n > 0) : (n /= base) reversed = reversed * base + n % base;
    return number == reversed;
}

fn printAnswer(value: anytype) !void {
    var buffer: [64]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
    try std.Io.File.stdout().writeStreamingAll(std.Options.debug_io, text);
}
