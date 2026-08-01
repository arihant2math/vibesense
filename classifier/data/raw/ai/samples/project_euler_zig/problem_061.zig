const std = @import("std");

// Project Euler 61: cyclical four-digit polygonal numbers.
const Candidate = struct { value: u16, prefix: u8, suffix: u8 };

fn polygonal(kind: usize, n: u32) u32 {
    return switch (kind) {
        0 => n * (n + 1) / 2,
        1 => n * n,
        2 => n * (3 * n - 1) / 2,
        3 => n * (2 * n - 1),
        4 => n * (5 * n - 3) / 2,
        else => n * (3 * n - 2),
    };
}

fn search(lists: *const [6][100]Candidate, counts: *const [6]usize, chosen: *[6]Candidate, used: [6]bool, depth: usize) bool {
    if (depth == 6) return chosen[5].suffix == chosen[0].prefix;
    for (0..6) |kind| {
        if (used[kind]) continue;
        for (lists[kind][0..counts[kind]]) |candidate| {
            if (depth != 0 and candidate.prefix != chosen[depth - 1].suffix) continue;
            var next_used = used;
            next_used[kind] = true;
            chosen[depth] = candidate;
            if (search(lists, counts, chosen, next_used, depth + 1)) return true;
        }
    }
    return false;
}

pub fn main() !void {
    var lists: [6][100]Candidate = undefined;
    var counts = [6]usize{ 0, 0, 0, 0, 0, 0 };
    for (0..6) |kind| {
        for (1..200) |n| {
            const value = polygonal(kind, @intCast(n));
            if (value < 1000 or value >= 10000) continue;
            const suffix: u8 = @intCast(value % 100);
            if (suffix < 10) continue;
            lists[kind][counts[kind]] = .{ .value = @intCast(value), .prefix = @intCast(value / 100), .suffix = suffix };
            counts[kind] += 1;
        }
    }
    var chosen: [6]Candidate = undefined;
    const found = search(&lists, &counts, &chosen, .{ false, false, false, false, false, false }, 0);
    if (!found) return error.NoSolution;
    var total: u32 = 0;
    for (chosen) |candidate| total += candidate.value;
    var buffer: [32]u8 = undefined;
    const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{total});
    _ = std.c.write(1, text.ptr, text.len);
}
