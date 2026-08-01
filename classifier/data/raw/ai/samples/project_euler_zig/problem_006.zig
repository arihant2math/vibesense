const std = @import("std");

// Project Euler Problem 6: difference between the square of the sum and sum of squares for 1 through 100.
pub fn main() !void {
    var sum: u64 = 0;
    var sum_of_squares: u64 = 0;
    var n: u64 = 1;
    while (n <= 100) : (n += 1) {
        sum += n;
        sum_of_squares += n * n;
    }
    try printNumber(sum * sum - sum_of_squares);
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
