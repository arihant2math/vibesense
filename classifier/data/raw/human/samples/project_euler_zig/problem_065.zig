const std = @import("std");

// Project Euler 65: digit sum of the numerator of e's 100th convergent.
const Big = struct {
    digits: [128]u8,
    len: usize,

    fn init(value_in: u32) Big {
        var value = value_in;
        var result = Big{ .digits = undefined, .len = 0 };
        if (value == 0) {
            result.digits[0] = 0;
            result.len = 1;
            return result;
        }
        while (value != 0) : (value /= 10) {
            result.digits[result.len] = @intCast(value % 10);
            result.len += 1;
        }
        return result;
    }

    fn add(left: Big, right: Big) Big {
        var result = Big{ .digits = undefined, .len = 0 };
        var carry: u32 = 0;
        const length = @max(left.len, right.len);
        var i: usize = 0;
        while (i < length or carry != 0) : (i += 1) {
            const l: u32 = if (i < left.len) left.digits[i] else 0;
            const r: u32 = if (i < right.len) right.digits[i] else 0;
            const sum = l + r + carry;
            result.digits[i] = @intCast(sum % 10);
            carry = sum / 10;
            result.len = i + 1;
        }
        return result;
    }

    fn mulSmall(value: Big, multiplier: u32) Big {
        var result = Big{ .digits = undefined, .len = 0 };
        var carry: u32 = 0;
        var i: usize = 0;
        while (i < value.len or carry != 0) : (i += 1) {
            const digit: u32 = if (i < value.len) value.digits[i] else 0;
            const product = digit * multiplier + carry;
            result.digits[i] = @intCast(product % 10);
            carry = product / 10;
            result.len = i + 1;
        }
        return result;
    }
};

pub fn main() !void {
    var p_minus_two = Big.init(0);
    var p_minus_one = Big.init(1);
    var numerator = Big.init(0);
    for (0..100) |index| {
        const term: u32 = if (index == 0) 2 else if (index % 3 == 2) @intCast(2 * (index / 3 + 1)) else 1;
        numerator = Big.add(Big.mulSmall(p_minus_one, term), p_minus_two);
        p_minus_two = p_minus_one;
        p_minus_one = numerator;
    }
    var sum: u32 = 0;
    for (numerator.digits[0..numerator.len]) |digit| sum += digit;
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{sum});
    _ = std.c.write(1, text.ptr, text.len);
}
