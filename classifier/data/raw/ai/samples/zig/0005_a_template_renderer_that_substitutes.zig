const std = @import("std");

pub const Value = struct {
    name: []const u8,
    value: []const u8,
};

pub fn render(
    allocator: std.mem.Allocator,
    template: []const u8,
    values: []const Value,
) ![]u8 {
    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();

    var index: usize = 0;

    while (index < template.len) {
        const start = std.mem.indexOfPos(u8, template, index, "{{") orelse {
            try result.appendSlice(template[index..]);
            break;
        };

        try result.appendSlice(template[index..start]);

        const end = std.mem.indexOfPos(u8, template, start + 2, "}}") orelse {
            try result.appendSlice(template[start..]);
            break;
        };

        const name = std.mem.trim(u8, template[start + 2 .. end], " \t\r\n");
        var replaced = false;

        for (values) |entry| {
            if (std.mem.eql(u8, name, entry.name)) {
                try result.appendSlice(entry.value);
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            try result.appendSlice(template[start .. end + 2]);
        }

        index = end + 2;
    }

    return result.toOwnedSlice();
}

test "renders placeholders" {
    const allocator = std.testing.allocator;

    const values = [_]Value{
        .{ .name = "name", .value = "Alice" },
        .{ .name = "language", .value = "Zig" },
    };

    const output = try render(
        allocator,
        "Hello, {{ name }}! Welcome to {{language}}.",
        &values,
    );
    defer allocator.free(output);

    try std.testing.expectEqualStrings(
        "Hello, Alice! Welcome to Zig.",
        output,
    );
}

test "preserves unknown placeholders" {
    const allocator = std.testing.allocator;

    const values = [_]Value{
        .{ .name = "name", .value = "Alice" },
    };

    const output = try render(
        allocator,
        "{{name}} {{unknown}}",
        &values,
    );
    defer allocator.free(output);

    try std.testing.expectEqualStrings(
        "Alice {{unknown}}",
        output,
    );
}
