const std = @import("std");

// Project Euler 62: smallest cube with exactly five cubic permutations.
const Group = struct { key: u64, first: u64, count: u8 };

fn digitKey(value_in: u64) u64 {
    var value = value_in;
    var counts = [10]u64{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    while (value != 0) : (value /= 10) counts[value % 10] += 1;
    var key: u64 = 0;
    for (counts) |count| key = key * 16 + count;
    return key;
}

fn digits(value_in: u64) u32 {
    var value = value_in;
    var result: u32 = 0;
    while (value != 0) : (value /= 10) result += 1;
    return result;
}

pub fn main() !void {
    var groups: [10_000]Group = undefined;
    var group_count: usize = 0;
    var current_digits: u32 = 1;
    for (1..10_001) |root_usize| {
        const root: u64 = @intCast(root_usize);
        const cube = root * root * root;
        const cube_digits = digits(cube);
        if (cube_digits != current_digits) {
            var answer: ?u64 = null;
            for (groups[0..group_count]) |group| {
                if (group.count == 5 and (answer == null or group.first < answer.?)) answer = group.first;
            }
            if (answer) |value| {
                var buffer: [32]u8 = undefined;
                const text = try std.fmt.bufPrint(&buffer, "{d}\n", .{value});
                _ = std.c.write(1, text.ptr, text.len);
                return;
            }
            group_count = 0;
            current_digits = cube_digits;
        }
        const key = digitKey(cube);
        var index: usize = 0;
        while (index < group_count and groups[index].key != key) : (index += 1) {}
        if (index == group_count) {
            groups[group_count] = .{ .key = key, .first = cube, .count = 1 };
            group_count += 1;
        } else {
            groups[index].count += 1;
        }
    }
    return error.NoSolution;
}
