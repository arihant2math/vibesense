const std = @import("std");

// Project Euler 90: arrangements of two digit cubes that display all square numbers.
fn hasDigit(cube: u16, digit: usize) bool {
    if (digit == 6 or digit == 9) return (cube & ((@as(u16, 1) << 6) | (@as(u16, 1) << 9))) != 0;
    return (cube & (@as(u16, 1) << @intCast(digit))) != 0;
}

fn canDisplay(first: u16, second: u16, left: usize, right: usize) bool {
    return (hasDigit(first, left) and hasDigit(second, right)) or
        (hasDigit(first, right) and hasDigit(second, left));
}

fn displaysSquares(first: u16, second: u16) bool {
    const squares = [_][2]usize{ .{ 0, 1 }, .{ 0, 4 }, .{ 0, 9 }, .{ 1, 6 }, .{ 2, 5 }, .{ 3, 6 }, .{ 4, 9 }, .{ 6, 4 }, .{ 8, 1 } };
    for (squares) |pair| {
        if (!canDisplay(first, second, pair[0], pair[1])) return false;
    }
    return true;
}

pub fn main() !void {
    var cubes: [210]u16 = undefined;
    var count: usize = 0;
    var a: usize = 0;
    while (a < 5) : (a += 1) {
        var b = a + 1;
        while (b < 6) : (b += 1) {
            var c = b + 1;
            while (c < 7) : (c += 1) {
                var d = c + 1;
                while (d < 8) : (d += 1) {
                    var e = d + 1;
                    while (e < 9) : (e += 1) {
                        var f = e + 1;
                        while (f < 10) : (f += 1) {
                            cubes[count] = (@as(u16, 1) << @intCast(a)) | (@as(u16, 1) << @intCast(b)) | (@as(u16, 1) << @intCast(c)) | (@as(u16, 1) << @intCast(d)) | (@as(u16, 1) << @intCast(e)) | (@as(u16, 1) << @intCast(f));
                            count += 1;
                        }
                    }
                }
            }
        }
    }

    var arrangements: usize = 0;
    var i: usize = 0;
    while (i < count) : (i += 1) {
        var j = i;
        while (j < count) : (j += 1) {
            if (displaysSquares(cubes[i], cubes[j])) arrangements += 1;
        }
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d}\n", .{arrangements});
    try writer.interface.flush();
}
