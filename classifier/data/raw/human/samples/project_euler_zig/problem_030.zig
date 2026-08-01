// Project Euler 030: sum numbers equal to the sum of fifth powers of their digits.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

pub fn main() !void {
    const fifth = [_]u32{ 0, 1, 32, 243, 1_024, 3_125, 7_776, 16_807, 32_768, 59_049 };
    var total: u32 = 0;
    var n: u32 = 2;
    while (n <= 354_294) : (n += 1) {
        var remaining = n;
        var digit_sum: u32 = 0;
        while (remaining > 0) : (remaining /= 10) {
            digit_sum += fifth[remaining % 10];
        }
        if (digit_sum == n) total += n;
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = total;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
