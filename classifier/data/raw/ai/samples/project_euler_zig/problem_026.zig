// Project Euler 026: denominator below 1,000 with the longest recurring cycle.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

pub fn main() !void {
    var best_denominator: usize = 0;
    var longest: i32 = 0;
    var d: usize = 2;
    while (d < 1_000) : (d += 1) {
        var first_seen: [1_000]i32 = undefined;
        for (&first_seen) |*position| position.* = -1;
        var remainder: usize = 1 % d;
        var position: i32 = 0;
        while (remainder != 0 and first_seen[remainder] == -1) {
            first_seen[remainder] = position;
            remainder = (remainder * 10) % d;
            position += 1;
        }
        if (remainder != 0) {
            const cycle = position - first_seen[remainder];
            if (cycle > longest) {
                longest = cycle;
                best_denominator = d;
            }
        }
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = best_denominator;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
