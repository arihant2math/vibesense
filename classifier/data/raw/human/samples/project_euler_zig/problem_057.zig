const std = @import("std");
// Project Euler 57: square-root-two convergents with a longer numerator.
const Big = struct {
    digits: [400]u8 = [_]u8{0} ** 400,
    len: usize = 1,
    fn from(n0: u32) Big {
        var x = Big{};
        var n = n0;
        x.len = 0;
        while (n != 0) : (n /= 10) {
            x.digits[x.len] = @intCast(n % 10);
            x.len += 1;
        }
        return x;
    }
    fn add(a: Big, b: Big) Big {
        var r = Big{};
        r.len = @max(a.len, b.len);
        var carry: u8 = 0;
        for (0..r.len) |i| {
            const v: u8 = (if (i < a.len) a.digits[i] else 0) + (if (i < b.len) b.digits[i] else 0) + carry;
            r.digits[i] = v % 10;
            carry = v / 10;
        }
        if (carry != 0) {
            r.digits[r.len] = carry;
            r.len += 1;
        }
        return r;
    }
    fn twice(a: Big) Big {
        return a.add(a);
    }
};
pub fn main() !void {
    var numerator = Big.from(3);
    var denominator = Big.from(2);
    var count: u32 = 0;
    for (0..1000) |_| {
        if (numerator.len > denominator.len) count += 1;
        const next_n = numerator.add(denominator.twice());
        denominator = numerator.add(denominator);
        numerator = next_n;
    }
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var out: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &out);
    try w.interface.print("{d}\n", .{count});
    try w.interface.flush();
}
