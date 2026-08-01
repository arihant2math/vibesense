// Project Euler 023: sum positive integers not expressible as two abundant numbers.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

fn printNumber(n: usize) void {
    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = n;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
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
    const limit = 28_123;
    var abundant: [limit]usize = undefined;
    var count: usize = 0;
    var n: usize = 12;
    while (n <= limit) : (n += 1) {
        if (properDivisorSum(n) > n) {
            abundant[count] = n;
            count += 1;
        }
    }

    var expressible = [_]bool{false} ** (limit + 1);
    var i: usize = 0;
    while (i < count) : (i += 1) {
        var j = i;
        while (j < count) : (j += 1) {
            const sum = abundant[i] + abundant[j];
            if (sum > limit) break;
            expressible[sum] = true;
        }
    }

    var total: usize = 0;
    n = 1;
    while (n <= limit) : (n += 1) {
        if (!expressible[n]) total += n;
    }
    printNumber(total);
}
