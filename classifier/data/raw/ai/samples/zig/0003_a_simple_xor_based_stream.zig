const std = @import("std");

const RotatingXor = struct {
    key: []const u8,
    offset: usize = 0,

    pub fn apply(self: *RotatingXor, data: []u8) void {
        if (self.key.len == 0) return;

        for (data, 0..) |*byte, index| {
            const key_index = (self.offset + index) % self.key.len;
            byte.* ^= self.key[key_index];
        }

        self.offset = (self.offset + data.len) % self.key.len;
    }
};

pub fn main() void {
    var message = [_]u8{
        'X', 'O', 'R', ' ', 'c', 'i', 'p', 'h', 'e', 'r', ' ', 'd', 'e', 'm', 'o',
    };
    const original = message;

    var cipher = RotatingXor{ .key = "rotating-key" };

    cipher.apply(message[0..]);

    std.debug.print("Encrypted: ", .{});
    for (message) |byte| {
        std.debug.print("{x:0>2} ", .{byte});
    }
    std.debug.print("\n", .{});

    cipher.apply(message[0..]);

    std.debug.print("Decrypted: {s}\n", .{message});
    std.debug.print("Matches original: {}\n", .{std.mem.eql(u8, &message, &original)});
}
