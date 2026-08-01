const std = @import("std");

// Project Euler 95: smallest member of the longest amicable chain below one million.
pub fn main() !void {
    const limit: usize = 1_000_000;
    const allocator = std.heap.page_allocator;
    const sums = try allocator.alloc(usize, limit + 1);
    defer allocator.free(sums);
    @memset(sums, 0);
    for (1..limit / 2 + 1) |divisor| {
        var multiple = divisor * 2;
        while (multiple <= limit) : (multiple += divisor) sums[multiple] += divisor;
    }

    const completed = try allocator.alloc(bool, limit + 1);
    defer allocator.free(completed);
    @memset(completed, false);
    const seen_run = try allocator.alloc(u32, limit + 1);
    defer allocator.free(seen_run);
    @memset(seen_run, 0);
    const depth = try allocator.alloc(usize, limit + 1);
    defer allocator.free(depth);

    var run: u32 = 0;
    var best_length: usize = 0;
    var answer: usize = 0;
    for (1..limit + 1) |start| {
        if (completed[start]) continue;
        run += 1;
        var value = start;
        var length: usize = 0;
        while (value <= limit and !completed[value] and seen_run[value] != run) {
            seen_run[value] = run;
            depth[value] = length;
            length += 1;
            value = sums[value];
        }
        if (value <= limit and seen_run[value] == run) {
            const cycle_length = length - depth[value];
            if (cycle_length > best_length) {
                best_length = cycle_length;
                answer = value;
                var item = sums[value];
                while (item != value) : (item = sums[item]) answer = @min(answer, item);
            }
        }
        value = start;
        while (value <= limit and !completed[value] and seen_run[value] == run) {
            completed[value] = true;
            value = sums[value];
        }
    }
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{answer})));
}
