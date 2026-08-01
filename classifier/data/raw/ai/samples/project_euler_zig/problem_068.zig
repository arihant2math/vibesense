const std = @import("std");

// Project Euler 68: maximum 16-digit string for a magic 5-gon ring.
fn take(x: i32, used: *[11]bool) bool {
    if (x < 1 or x > 10) return false;
    const index: usize = @intCast(x);
    if (used[index]) return false;
    used[index] = true;
    return true;
}

fn append(value: u64, number: i32) u64 {
    return if (number == 10) value * 100 + 10 else value * 10 + @as(u64, @intCast(number));
}

pub fn main() !void {
    var maximum: u64 = 0;
    for (1..11) |a_u| for (1..11) |b_u| for (1..11) |c_u| for (1..11) |d_u| for (1..11) |e_u| {
        const a: i32 = @intCast(a_u);
        const b: i32 = @intCast(b_u);
        const c: i32 = @intCast(c_u);
        const d: i32 = @intCast(d_u);
        const e: i32 = @intCast(e_u);
        if (a == b or a == c or a == d or a == e or b == c or b == d or b == e or c == d or c == e or d == e) continue;
        for (1..11) |outer_u| {
            const outer1: i32 = @intCast(outer_u);
            var used = [_]bool{false} ** 11;
            if (!take(a, &used) or !take(b, &used) or !take(c, &used) or !take(d, &used) or !take(e, &used) or !take(outer1, &used)) continue;
            const line_sum = outer1 + a + b;
            const outer2 = line_sum - b - c;
            const outer3 = line_sum - c - d;
            const outer4 = line_sum - d - e;
            const outer5 = line_sum - e - a;
            if (!take(outer2, &used) or !take(outer3, &used) or !take(outer4, &used) or !take(outer5, &used)) continue;
            if (outer1 > outer2 or outer1 > outer3 or outer1 > outer4 or outer1 > outer5) continue;
            var value: u64 = 0;
            value = append(value, outer1); value = append(value, a); value = append(value, b);
            value = append(value, outer2); value = append(value, b); value = append(value, c);
            value = append(value, outer3); value = append(value, c); value = append(value, d);
            value = append(value, outer4); value = append(value, d); value = append(value, e);
            value = append(value, outer5); value = append(value, e); value = append(value, a);
            if (value >= 1_000_000_000_000_000 and value < 10_000_000_000_000_000 and value > maximum) maximum = value;
        }
    };
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{maximum});
    _ = std.c.write(1, text.ptr, text.len);
}
