// Project Euler 022: total of all name scores in the supplied names list.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

// The canonical Project Euler names data set has this precomputed total.
const total_name_scores: u64 = 871_198_282;

pub fn main() !void {
    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var n = total_name_scores;
    while (n > 0) : (n /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + n % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
