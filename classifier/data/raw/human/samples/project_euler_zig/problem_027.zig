// Project Euler 027: product of coefficients producing the most consecutive primes.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

fn isPrime(n: i64) bool {
    if (n < 2) return false;
    if (n == 2) return true;
    if (@mod(n, 2) == 0) return false;
    var divisor: i64 = 3;
    while (divisor <= @divTrunc(n, divisor)) : (divisor += 2) {
        if (@mod(n, divisor) == 0) return false;
    }
    return true;
}

fn printNumber(value: i64) void {
    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var n: u64 = if (value < 0) @intCast(-value) else @intCast(value);
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

pub fn main() !void {
    var best_length: i64 = 0;
    var best_product: i64 = 0;
    var a: i64 = -999;
    while (a <= 999) : (a += 1) {
        var b: i64 = 2;
        while (b <= 999) : (b += 1) {
            if (!isPrime(b)) continue;
            var n: i64 = 0;
            while (isPrime(n * n + a * n + b)) : (n += 1) {}
            if (n > best_length) {
                best_length = n;
                best_product = a * b;
            }
        }
    }
    printNumber(best_product);
}
