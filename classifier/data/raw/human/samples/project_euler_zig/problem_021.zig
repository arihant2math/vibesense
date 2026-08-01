// Project Euler 021: sum amicable numbers below ten thousand.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

fn printNumber(value: i64) void {
    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var n: u64 = if (value < 0) @intCast(-value) else @intCast(value);
    if (n == 0) {
        pos -= 1;
        buffer[pos] = '0';
    }
    while (n > 0) : (n /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + n % 10);
    }
    if (value < 0) {
        pos -= 1;
        buffer[pos] = '-';
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}

fn properDivisorSum(n: usize) usize {
    if (n < 2) return 0;
    var sum: usize = 1;
    var d: usize = 2;
    while (d <= n / d) : (d += 1) {
        if (n % d == 0) {
            sum += d;
            const other = n / d;
            if (other != d) sum += other;
        }
    }
    return sum;
}

pub fn main() !void {
    var total: usize = 0;
    var a: usize = 2;
    while (a < 10_000) : (a += 1) {
        const b = properDivisorSum(a);
        if (b != a and properDivisorSum(b) == a) total += a;
    }
    printNumber(@intCast(total));
}
