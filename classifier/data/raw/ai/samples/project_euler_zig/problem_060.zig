const std = @import("std");
// Project Euler 60: minimum-sum set of five primes whose pairwise concatenations are prime.
const max_prime = 10_000;
var composite: [max_prime + 1]bool = undefined;
var primes: [1300]u32 = undefined;
var prime_count: usize = 0;
var compatible: [1300][1300]bool = undefined;
var selected: [5]usize = undefined;
var best: u32 = 1_000_000;

fn isPrime(n: u64) bool {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    var d: u64 = 3;
    while (d <= n / d) : (d += 2) if (n % d == 0) return false;
    return true;
}
fn concatenate(a: u32, b: u32) u64 {
    var scale: u64 = 10;
    var x = b;
    while (x >= 10) : (x /= 10) scale *= 10;
    return @as(u64, a) * scale + b;
}
fn pairOK(i: usize, j: usize) bool {
    const lo = @min(i, j);
    const hi = @max(i, j);
    return compatible[lo][hi];
}
fn search(depth: usize, start: usize, sum: u32) void {
    if (depth == 5) {
        best = @min(best, sum);
        return;
    }
    const remaining = 5 - depth;
    var i = start;
    while (i < prime_count) : (i += 1) {
        if (sum + primes[i] * remaining >= best) break;
        var ok = true;
        for (0..depth) |j| if (!pairOK(selected[j], i)) {
            ok = false;
            break;
        };
        if (!ok) continue;
        selected[depth] = i;
        search(depth + 1, i + 1, sum + primes[i]);
    }
}
pub fn main() !void {
    @memset(&composite, false);
    composite[0] = true;
    composite[1] = true;
    for (2..max_prime + 1) |p| if (!composite[p]) {
        if (p <= max_prime / p) {
            var m = p * p;
            while (m <= max_prime) : (m += p) composite[m] = true;
        }
        if (p != 2 and p != 5) {
            primes[prime_count] = @intCast(p);
            prime_count += 1;
        }
    };
    for (&compatible) |*row| @memset(row, false);
    for (0..prime_count) |i| {
        for (i + 1..prime_count) |j| {
            if (isPrime(concatenate(primes[i], primes[j])) and isPrime(concatenate(primes[j], primes[i]))) {
                compatible[i][j] = true;
            }
        }
    }
    search(0, 0, 0);
    var threaded = std.Io.Threaded.init_single_threaded;
    const io = threaded.io();
    var out: [64]u8 = undefined;
    var w = std.Io.File.stdout().writer(io, &out);
    try w.interface.print("{d}\n", .{best});
    try w.interface.flush();
}
