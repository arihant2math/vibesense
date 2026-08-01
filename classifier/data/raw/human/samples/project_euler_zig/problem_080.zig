// Project Euler Problem 80: digital sums of the first 100 decimal digits of irrational square roots.
const std = @import("std");

const limb_count = 32;
const base: u64 = 1_000_000_000;

const Big = struct {
    limbs: [limb_count]u32 = [_]u32{0} ** limb_count,

    fn mulSmall(self: *Big, factor: u32) void {
        var carry: u64 = 0;
        for (&self.limbs) |*limb| {
            const product = @as(u64, limb.*) * factor + carry;
            limb.* = @intCast(product % base);
            carry = product / base;
        }
        std.debug.assert(carry == 0);
    }

    fn addSmall(self: *Big, value: u32) void {
        var carry: u64 = value;
        for (&self.limbs) |*limb| {
            const total = @as(u64, limb.*) + carry;
            limb.* = @intCast(total % base);
            carry = total / base;
            if (carry == 0) break;
        }
        std.debug.assert(carry == 0);
    }

    fn cmp(left: Big, right: Big) std.math.Order {
        var i: usize = limb_count;
        while (i > 0) {
            i -= 1;
            if (left.limbs[i] < right.limbs[i]) return .lt;
            if (left.limbs[i] > right.limbs[i]) return .gt;
        }
        return .eq;
    }

    fn sub(self: *Big, other: Big) void {
        var borrow: i64 = 0;
        for (0..limb_count) |i| {
            const difference = @as(i64, @intCast(self.limbs[i])) -
                @as(i64, @intCast(other.limbs[i])) - borrow;
            if (difference < 0) {
                self.limbs[i] = @intCast(difference + @as(i64, base));
                borrow = 1;
            } else {
                self.limbs[i] = @intCast(difference);
                borrow = 0;
            }
        }
        std.debug.assert(borrow == 0);
    }
};

fn isSquare(n: u32) bool {
    var root: u32 = 1;
    while (root * root < n) : (root += 1) {}
    return root * root == n;
}

pub fn main() !void {
    var total: u64 = 0;
    var number: u32 = 1;
    while (number <= 100) : (number += 1) {
        if (isSquare(number)) continue;
        var root = Big{};
        var remainder = Big{};
        var digit_sum: u64 = 0;

        // Longhand square root consumes the integer group then 99 groups of 00.
        for (0..100) |step| {
            remainder.mulSmall(100);
            if (step == 0) remainder.addSmall(number);

            var chosen: u32 = 0;
            var candidate: u32 = 0;
            while (candidate <= 9) : (candidate += 1) {
                var trial = root;
                trial.mulSmall(20);
                trial.addSmall(candidate);
                trial.mulSmall(candidate);
                if (trial.cmp(remainder) == .gt) break;
                chosen = candidate;
            }
            var subtrahend = root;
            subtrahend.mulSmall(20);
            subtrahend.addSmall(chosen);
            subtrahend.mulSmall(chosen);
            remainder.sub(subtrahend);
            root.mulSmall(10);
            root.addSmall(chosen);
            digit_sum += chosen;
        }
        total += digit_sum;
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{total});
    try out.interface.flush();
}
