// Project Euler 028: sum of the diagonals in a 1001 by 1001 number spiral.
extern "c" fn write(fd: c_int, buf: [*]const u8, count: usize) isize;

pub fn main() !void {
    var sum: u64 = 1;
    var corner: u64 = 1;
    var layer: u64 = 1;
    while (layer <= 500) : (layer += 1) {
        const step = layer * 2;
        var count: u8 = 0;
        while (count < 4) : (count += 1) {
            corner += step;
            sum += corner;
        }
    }

    var buffer: [32]u8 = undefined;
    var pos = buffer.len - 1;
    buffer[pos] = '\n';
    var value = sum;
    while (value > 0) : (value /= 10) {
        pos -= 1;
        buffer[pos] = @intCast('0' + value % 10);
    }
    _ = write(1, buffer[pos..].ptr, buffer.len - pos);
}
