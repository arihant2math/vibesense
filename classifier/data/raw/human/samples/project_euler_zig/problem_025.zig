// Project Euler 025: index of the first Fibonacci number with 1,000 digits.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

const base: u64 = 1_000_000_000;
const limbs = 112;

fn decimalDigits(n: u32) usize {
    var value = n;
    var count: usize = 1;
    while (value >= 10) : (value /= 10) count += 1;
    return count;
}

fn digitCount(number: [limbs]u32) usize {
    var i: usize = limbs;
    while (i > 0) {
        i -= 1;
        if (number[i] != 0) return i * 9 + decimalDigits(number[i]);
    }
    return 1;
}

pub fn main() !void {
    var previous = [_]u32{0} ** limbs;
    var current = [_]u32{0} ** limbs;
    previous[0] = 1; // F(1)
    current[0] = 1; // F(2)
    var index: usize = 2;

    while (digitCount(current) < 1_000) {
        var next = [_]u32{0} ** limbs;
        var carry: u64 = 0;
        var i: usize = 0;
        while (i < limbs) : (i += 1) {
            const sum: u64 = @as(u64, previous[i]) + current[i] + carry;
            next[i] = @intCast(sum % base);
            carry = sum / base;
        }
        previous = current;
        current = next;
        index += 1;
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = index;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
