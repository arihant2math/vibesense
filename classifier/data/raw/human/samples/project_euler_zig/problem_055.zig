const std = @import("std");

// Project Euler 55: count numbers below ten thousand that appear to be Lychrel numbers.
const Decimal = struct {
    // Least-significant decimal digit first; 128 digits is ample for 50 iterations here.
    digit: [128]u8 = [_]u8{0} ** 128,
    len: usize = 1,

    fn from(n0: u32) Decimal {
        var result = Decimal{};
        var n = n0;
        result.len = 0;
        while (n != 0) : (n /= 10) {
            result.digit[result.len] = @intCast(n % 10);
            result.len += 1;
        }
        return result;
    }

    fn addReverse(self: *Decimal) void {
        const old = self.*;
        var carry: u8 = 0;
        for (0..old.len) |i| {
            const value = old.digit[i] + old.digit[old.len - 1 - i] + carry;
            self.digit[i] = value % 10;
            carry = value / 10;
        }
        self.len = old.len;
        if (carry != 0) {
            self.digit[self.len] = carry;
            self.len += 1;
        }
    }

    fn isPalindrome(self: Decimal) bool {
        for (0..self.len / 2) |i| {
            if (self.digit[i] != self.digit[self.len - 1 - i]) return false;
        }
        return true;
    }
};

pub fn main() !void {
    var count: u32 = 0;
    for (1..10_000) |start| {
        var value = Decimal.from(@intCast(start));
        var is_lychrel = true;
        for (0..50) |_| {
            value.addReverse();
            if (value.isPalindrome()) {
                is_lychrel = false;
                break;
            }
        }
        if (is_lychrel) count += 1;
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(io, &buffer);
    try writer.interface.print("{d}\n", .{count});
    try writer.interface.flush();
}
