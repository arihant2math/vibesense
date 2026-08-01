// Project Euler Problem 74: digit-factorial chains containing exactly sixty terms.
const std = @import("std");

fn next(value: usize, factorial: *const [10]usize) usize {
    if (value == 0) return factorial[0];
    var n = value;
    var result: usize = 0;
    while (n > 0) : (n /= 10) result += factorial[n % 10];
    return result;
}

pub fn main() !void {
    const factorial = [_]usize{ 1, 1, 2, 6, 24, 120, 720, 5_040, 40_320, 362_880 };
    const max_value = 7 * factorial[9];
    const seen = try std.heap.page_allocator.alloc(u32, max_value + 1);
    defer std.heap.page_allocator.free(seen);
    @memset(seen, 0);

    var answer: u32 = 0;
    var start: u32 = 1;
    while (start < 1_000_000) : (start += 1) {
        var value: usize = start;
        var length: u32 = 0;
        while (seen[value] != start) {
            seen[value] = start;
            length += 1;
            value = next(value, &factorial);
        }
        if (length == 60) answer += 1;
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{answer});
    try out.interface.flush();
}
