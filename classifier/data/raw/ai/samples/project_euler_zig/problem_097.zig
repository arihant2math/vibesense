const std = @import("std");

// Project Euler 97: last ten digits of the specified non-Mersenne prime.
fn multiplyMod(a: u64, b: u64, modulus: u64) u64 {
    return @intCast((@as(u128, a) * @as(u128, b)) % modulus);
}

fn powMod(mut_base: u64, mut_exponent: u64, modulus: u64) u64 {
    var base = mut_base;
    var exponent = mut_exponent;
    var result: u64 = 1;
    while (exponent != 0) : (exponent >>= 1) {
        if ((exponent & 1) != 0) result = multiplyMod(result, base, modulus);
        base = multiplyMod(base, base, modulus);
    }
    return result;
}

pub fn main() !void {
    const modulus: u64 = 10_000_000_000;
    const answer = (28_433 * powMod(2, 7_830_457, modulus) + 1) % modulus;
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d:0>10}\n", .{answer})));
}
