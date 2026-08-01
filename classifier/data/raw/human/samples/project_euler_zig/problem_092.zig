const std = @import("std");

// Project Euler 92: count starting values below ten million ending at 89.
fn squareDigitSum(mut_value: usize) usize {
    var value = mut_value;
    var sum: usize = 0;
    while (value != 0) : (value /= 10) {
        const digit = value % 10;
        sum += digit * digit;
    }
    return sum;
}

pub fn main() !void {
    var fate = [_]u8{0} ** 568; // A seven-digit number has sum at most 7 * 81.
    fate[1] = 1;
    fate[89] = 89;
    var count: usize = 0;
    for (1..10_000_000) |start| {
        var value = squareDigitSum(start);
        var trail: [32]usize = undefined;
        var trail_len: usize = 0;
        while (fate[value] == 0) {
            trail[trail_len] = value;
            trail_len += 1;
            value = squareDigitSum(value);
        }
        const ending = fate[value];
        for (trail[0..trail_len]) |item| fate[item] = ending;
        if (ending == 89) count += 1;
    }
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{count})));
}
