const std = @import("std");

// Project Euler 43: pandigital numbers with substring divisibility.
pub fn main() !void {
    var digits: [10]u8 = undefined;
    var used = [_]bool{false} ** 10;
    var total: u64 = 0;
    search(&digits, &used, 0, &total);
    printNumber(total);
}

fn search(digits: *[10]u8, used: *[10]bool, position: usize, total: *u64) void {
    const divisors = [_]u16{ 2, 3, 5, 7, 11, 13, 17 };
    if (position == 10) {
        var value: u64 = 0;
        for (digits) |digit| value = value * 10 + digit;
        total.* += value;
        return;
    }

    for (0..10) |candidate| {
        if (used[candidate] or (position == 0 and candidate == 0)) continue;
        digits[position] = @intCast(candidate);
        if (position >= 3) {
            const checked: u16 = @as(u16, digits[position - 2]) * 100 +
                @as(u16, digits[position - 1]) * 10 + digits[position];
            if (checked % divisors[position - 3] != 0) continue;
        }
        used[candidate] = true;
        search(digits, used, position + 1, total);
        used[candidate] = false;
    }
}

fn printNumber(value: anytype) void {
    var buffer: [64]u8 = undefined;
    const text = std.fmt.bufPrint(&buffer, "{d}\n", .{value}) catch return;
    _ = std.c.write(1, text.ptr, text.len);
}
