// Project Euler 029: count distinct terms in a^b for 2 <= a,b <= 100.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

const primes = [_]u16{ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };

fn sameExponents(left: [primes.len]u16, right: [primes.len]u16) bool {
    for (left, right) |a, b| {
        if (a != b) return false;
    }
    return true;
}

pub fn main() !void {
    var powers: [99 * 99][primes.len]u16 = undefined;
    var count: usize = 0;
    var a: u16 = 2;
    while (a <= 100) : (a += 1) {
        var b: u16 = 2;
        while (b <= 100) : (b += 1) {
            var exponents = [_]u16{0} ** primes.len;
            var remaining = a;
            for (primes, 0..) |prime, i| {
                while (remaining % prime == 0) {
                    exponents[i] += b;
                    remaining /= prime;
                }
            }
            var duplicate = false;
            var i: usize = 0;
            while (i < count) : (i += 1) {
                if (sameExponents(powers[i], exponents)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                powers[count] = exponents;
                count += 1;
            }
        }
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = count;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
