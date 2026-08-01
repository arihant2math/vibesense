// Project Euler 017: letters used when writing one to one thousand in English.
const std = @import("std");

fn lettersUnder100(n: u32) u32 {
    const ones = [_]u32{ 0, 3, 3, 5, 4, 4, 3, 5, 5, 4 };
    const teens = [_]u32{ 3, 6, 6, 8, 8, 7, 7, 9, 8, 8 };
    const tens = [_]u32{ 0, 0, 6, 6, 5, 5, 5, 7, 6, 6 };
    if (n < 10) return ones[n];
    if (n < 20) return teens[n - 10];
    return tens[n / 10] + ones[n % 10];
}

fn letters(n: u32) u32 {
    if (n == 1000) return 11; // one thousand
    if (n >= 100) return lettersUnder100(n / 100) + 7 + if (n % 100 == 0) 0 else 3 + lettersUnder100(n % 100);
    return lettersUnder100(n);
}

pub fn main() !void {
    var total: u32 = 0;
    for (1..1001) |n| total += letters(@intCast(n));
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{total});
    _ = std.c.write(1, text.ptr, text.len);
}
