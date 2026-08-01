const std = @import("std");

// Project Euler Problem 4: largest palindrome made from two three-digit numbers.
pub fn main() !void {
    var largest: u64 = 0;
    var first: u64 = 100;
    while (first <= 999) : (first += 1) {
        var second: u64 = first;
        while (second <= 999) : (second += 1) {
            const product = first * second;
            if (product > largest and isPalindrome(product)) largest = product;
        }
    }
    try printNumber(largest);
}

fn isPalindrome(number: u64) bool {
    var original = number;
    var reversed: u64 = 0;
    while (original > 0) : (original /= 10) {
        reversed = reversed * 10 + original % 10;
    }
    return number == reversed;
}

fn printNumber(number: anytype) !void {
    var buffer: [32]u8 = undefined;
    const output = try std.fmt.bufPrint(&buffer, "{d}\n", .{number});
    try std.Io.File.writeStreamingAll(.stdout(), std.Io.Threaded.global_single_threaded.io(), output);
}
