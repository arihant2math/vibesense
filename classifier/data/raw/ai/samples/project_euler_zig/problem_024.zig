// Project Euler 024: millionth lexicographic permutation of digits 0 through 9.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

fn factorial(n: usize) usize {
    var result: usize = 1;
    var i: usize = 2;
    while (i <= n) : (i += 1) result *= i;
    return result;
}

pub fn main() !void {
    var digits = [_]u8{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    var remaining: usize = digits.len;
    var rank: usize = 1_000_000 - 1; // zero-based rank
    var answer: u64 = 0;

    while (remaining > 0) : (remaining -= 1) {
        const block = factorial(remaining - 1);
        const pick = rank / block;
        rank %= block;
        answer = answer * 10 + digits[pick];
        var i = pick;
        while (i + 1 < remaining) : (i += 1) digits[i] = digits[i + 1];
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = answer;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
