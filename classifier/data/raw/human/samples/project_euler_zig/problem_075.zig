// Project Euler Problem 75: perimeters with exactly one integer right-triangle solution.
const std = @import("std");

fn gcd(a_initial: usize, b_initial: usize) usize {
    var a = a_initial;
    var b = b_initial;
    while (b != 0) {
        const remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

pub fn main() !void {
    const limit: usize = 1_500_000;
    const counts = try std.heap.page_allocator.alloc(u8, limit + 1);
    defer std.heap.page_allocator.free(counts);
    @memset(counts, 0);

    var m: usize = 2;
    while (2 * m * (m + 1) <= limit) : (m += 1) {
        var n: usize = 1;
        while (n < m) : (n += 1) {
            if ((m - n) % 2 == 0 or gcd(m, n) != 1) continue;
            const primitive_perimeter = 2 * m * (m + n);
            var perimeter = primitive_perimeter;
            while (perimeter <= limit) : (perimeter += primitive_perimeter) {
                if (counts[perimeter] < 2) counts[perimeter] += 1;
            }
        }
    }

    var answer: usize = 0;
    for (counts[1..]) |count| {
        if (count == 1) answer += 1;
    }
    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{answer});
    try out.interface.flush();
}
