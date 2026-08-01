// Project Euler Problem 79: derive the shortest passcode consistent with the keylog.
const std = @import("std");

pub fn main() !void {
    const keylog = [_][]const u8{
        "319", "680", "180", "690", "129", "620", "762", "689", "762", "318",
        "368", "710", "720", "710", "629", "168", "160", "689", "716", "731",
        "736", "729", "316", "729", "729", "710", "769", "290", "719", "680",
        "318", "389", "162", "289", "162", "718", "729", "319", "790", "680",
        "890", "362", "319", "760", "316", "729", "380", "319", "728", "716",
    };
    var edge = [_][10]bool{[_]bool{false} ** 10} ** 10;
    var present = [_]bool{false} ** 10;
    for (keylog) |entry| {
        for (entry) |character| present[character - '0'] = true;
        for (0..entry.len - 1) |i| edge[entry[i] - '0'][entry[i + 1] - '0'] = true;
    }

    var indegree = [_]u8{0} ** 10;
    for (0..10) |from| {
        for (0..10) |to| {
            if (edge[from][to]) indegree[to] += 1;
        }
    }

    var answer: u64 = 0;
    var selected = [_]bool{false} ** 10;
    var position: usize = 0;
    while (position < 10) : (position += 1) {
        var choice: ?usize = null;
        for (0..10) |digit| {
            if (present[digit] and !selected[digit] and indegree[digit] == 0) {
                choice = digit;
                break;
            }
        }
        const digit = choice orelse break;
        selected[digit] = true;
        answer = answer * 10 + digit;
        for (0..10) |to| {
            if (edge[digit][to]) indegree[to] -= 1;
        }
    }

    var buffer: [64]u8 = undefined;
    var out = std.Io.File.stdout().writer(std.Io.Threaded.global_single_threaded.io(), &buffer);
    try out.interface.print("{d}\n", .{answer});
    try out.interface.flush();
}
