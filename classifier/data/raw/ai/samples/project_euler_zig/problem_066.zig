const std = @import("std");

// Project Euler 66: D <= 1000 producing the largest minimal Pell equation x.
fn integerSqrt(n: u32) u32 {
    var root: u32 = 1;
    while ((root + 1) * (root + 1) <= n) : (root += 1) {}
    return root;
}

fn minimalX(d_in: u32) u128 {
    const d: u128 = d_in;
    const a0: u128 = integerSqrt(d_in);
    var m: u128 = 0;
    var denominator: u128 = 1;
    var a: u128 = a0;
    var p_minus_two: u128 = 1;
    var p_minus_one: u128 = a0;
    var period: u32 = 0;
    while (true) {
        m = denominator * a - m;
        denominator = (d - m * m) / denominator;
        a = (a0 + m) / denominator;
        const p = a * p_minus_one + p_minus_two;
        period += 1;
        // The final 2*a0 is one coefficient past the needed convergent
        // for an even period, and is the needed one for an odd period.
        if (a == 2 * a0) return if (period % 2 == 0) p_minus_one else p;
        p_minus_two = p_minus_one;
        p_minus_one = p;
    }
}

pub fn main() !void {
    var best_d: u32 = 0;
    var best_x: u128 = 0;
    for (2..1001) |d_usize| {
        const d: u32 = @intCast(d_usize);
        const root = integerSqrt(d);
        if (root * root == d) continue;
        const x = minimalX(d);
        if (x > best_x) {
            best_x = x;
            best_d = d;
        }
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{best_d});
    _ = std.c.write(1, text.ptr, text.len);
}
