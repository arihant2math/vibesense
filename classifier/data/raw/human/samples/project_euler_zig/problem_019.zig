// Project Euler 019: Sundays that fell on the first of a month during the twentieth century.
const std = @import("std");

fn leapYear(year: u32) bool {
    return year % 4 == 0 and (year % 100 != 0 or year % 400 == 0);
}

pub fn main() !void {
    const month_lengths = [_]u32{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    var weekday: u32 = 0; // Monday, 1 January 1900
    var count: u32 = 0;
    for (1900..2001) |year_value| {
        const year: u32 = @intCast(year_value);
        for (0..12) |month| {
            if (year >= 1901 and weekday == 6) count += 1;
            var days = month_lengths[month];
            if (month == 1 and leapYear(year)) days += 1;
            weekday = (weekday + days) % 7;
        }
    }
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{count});
    _ = std.c.write(1, text.ptr, text.len);
}
