const std = @import("std");

const Inventory = std.StringHashMap(i64);

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    const allocator = gpa.allocator();
    var inventory = Inventory.init(allocator);
    defer {
        var iterator = inventory.iterator();
        while (iterator.next()) |entry| {
            allocator.free(entry.key_ptr.*);
        }
        inventory.deinit();
    }

    const input = try std.io.getStdIn().readToEndAlloc(allocator, 1024 * 1024);
    defer allocator.free(input);

    var lines = std.mem.splitScalar(u8, input, '\n');
    var output = std.io.getStdOut().writer();

    while (lines.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \t\r");
        if (trimmed.len == 0) continue;

        var tokens = std.mem.tokenizeAny(u8, trimmed, " \t");
        const command = tokens.next() orelse continue;

        if (std.mem.eql(u8, command, "add")) {
            const name = tokens.next() orelse {
                try output.writeAll("Usage: add <item> <quantity>\n");
                continue;
            };
            const quantity_text = tokens.next() orelse {
                try output.writeAll("Usage: add <item> <quantity>\n");
                continue;
            };
            const quantity = std.fmt.parseInt(i64, quantity_text, 10) catch {
                try output.writeAll("Quantity must be a positive integer.\n");
                continue;
            };

            if (quantity <= 0) {
                try output.writeAll("Quantity must be a positive integer.\n");
                continue;
            }

            const result = try inventory.getOrPut(name);
            if (!result.found_existing) {
                result.key_ptr.* = try allocator.dupe(u8, name);
                result.value_ptr.* = 0;
            }
            result.value_ptr.* += quantity;

            try output.print("Added {d} {s}.\n", .{ quantity, name });
        } else if (std.mem.eql(u8, command, "remove")) {
            const name = tokens.next() orelse {
                try output.writeAll("Usage: remove <item> <quantity>\n");
                continue;
            };
            const quantity_text = tokens.next() orelse {
                try output.writeAll("Usage: remove <item> <quantity>\n");
                continue;
            };
            const quantity = std.fmt.parseInt(i64, quantity_text, 10) catch {
                try output.writeAll("Quantity must be a positive integer.\n");
                continue;
            };

            if (quantity <= 0) {
                try output.writeAll("Quantity must be a positive integer.\n");
                continue;
            }

            if (inventory.getPtr(name)) |current| {
                if (quantity >= current.*) {
                    if (inventory.fetchRemove(name)) |removed| {
                        allocator.free(removed.key);
                    }
                    try output.print("Removed all {s}.\n", .{name});
                } else {
                    current.* -= quantity;
                    try output.print("Removed {d} {s}.\n", .{ quantity, name });
                }
            } else {
                try output.print("{s} is not in the inventory.\n", .{name});
            }
        } else if (std.mem.eql(u8, command, "query")) {
            const name = tokens.next() orelse {
                try output.writeAll("Usage: query <item>\n");
                continue;
            };

            if (inventory.get(name)) |quantity| {
                try output.print("{s}: {d}\n", .{ name, quantity });
            } else {
                try output.print("{s}: 0\n", .{name});
            }
        } else if (std.mem.eql(u8, command, "list")) {
            var iterator = inventory.iterator();
            while (iterator.next()) |entry| {
                try output.print("{s}: {d}\n", .{ entry.key_ptr.*, entry.value_ptr.* });
            }
        } else {
            try output.writeAll("Commands: add, remove, query, list\n");
        }
    }
}
