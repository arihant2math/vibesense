const std = @import("std");

// Project Euler 93: four digits giving the longest run of positive arithmetic results.
const Fraction = struct { numerator: i64, denominator: i64 };

fn gcd(mut_a: i64, mut_b: i64) i64 {
    var a = if (mut_a < 0) -mut_a else mut_a;
    var b = if (mut_b < 0) -mut_b else mut_b;
    while (b != 0) {
        const remainder = @rem(a, b);
        a = b;
        b = remainder;
    }
    return a;
}

fn fraction(numerator: i64, denominator: i64) Fraction {
    var n = numerator;
    var d = denominator;
    if (d < 0) {
        n = -n;
        d = -d;
    }
    const divisor = gcd(n, d);
    return .{ .numerator = @divTrunc(n, divisor), .denominator = @divTrunc(d, divisor) };
}

fn add(a: Fraction, b: Fraction) Fraction {
    return fraction(a.numerator * b.denominator + b.numerator * a.denominator, a.denominator * b.denominator);
}
fn subtract(a: Fraction, b: Fraction) Fraction {
    return fraction(a.numerator * b.denominator - b.numerator * a.denominator, a.denominator * b.denominator);
}
fn multiply(a: Fraction, b: Fraction) Fraction {
    return fraction(a.numerator * b.numerator, a.denominator * b.denominator);
}
fn divide(a: Fraction, b: Fraction) Fraction {
    return fraction(a.numerator * b.denominator, a.denominator * b.numerator);
}

fn search(values: [4]Fraction, length: usize, possible: *[1000]bool) void {
    if (length == 1) {
        const value = values[0];
        if (value.numerator > 0 and @mod(value.numerator, value.denominator) == 0) {
            const integer = @divTrunc(value.numerator, value.denominator);
            if (integer < possible.len) possible[@intCast(integer)] = true;
        }
        return;
    }
    for (0..length) |left| {
        for ((left + 1)..length) |right| {
            var next: [4]Fraction = undefined;
            var next_length: usize = 0;
            for (0..length) |index| {
                if (index != left and index != right) {
                    next[next_length] = values[index];
                    next_length += 1;
                }
            }
            const a = values[left];
            const b = values[right];
            const results = [_]Fraction{
                add(a, b),                                              subtract(a, b),                                         subtract(b, a), multiply(a, b),
                if (b.numerator != 0) divide(a, b) else fraction(0, 1), if (a.numerator != 0) divide(b, a) else fraction(0, 1),
            };
            for (results, 0..) |result, operation| {
                if ((operation == 4 and b.numerator == 0) or (operation == 5 and a.numerator == 0)) continue;
                next[next_length] = result;
                search(next, next_length + 1, possible);
            }
        }
    }
}

pub fn main() !void {
    var best_run: usize = 0;
    var answer: usize = 0;
    for (1..10) |a| for ((a + 1)..10) |b| for ((b + 1)..10) |c| for ((c + 1)..10) |d| {
        var possible = [_]bool{false} ** 1000;
        search(.{
            fraction(@intCast(a), 1), fraction(@intCast(b), 1),
            fraction(@intCast(c), 1), fraction(@intCast(d), 1),
        }, 4, &possible);
        var run: usize = 1;
        while (run < possible.len and possible[run]) : (run += 1) {}
        if (run - 1 > best_run) {
            best_run = run - 1;
            answer = 1000 * a + 100 * b + 10 * c + d;
        }
    };
    var buffer: [32]u8 = undefined;
    try std.Io.File.writeStreamingAll(std.Io.File.stdout(), std.Io.Threaded.global_single_threaded.io(), (try std.fmt.bufPrint(&buffer, "{d}\n", .{answer})));
}
