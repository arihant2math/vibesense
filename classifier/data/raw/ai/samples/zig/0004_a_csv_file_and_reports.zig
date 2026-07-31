const std = @import("std");

const Stats = struct {
    count: usize = 0,
    missing: usize = 0,
    invalid: usize = 0,
    min: f64 = 0,
    max: f64 = 0,
    sum: f64 = 0,

    fn add(self: *Stats, value: ?f64) void {
        if (value) |number| {
            if (self.count == 0) {
                self.min = number;
                self.max = number;
            } else {
                if (number < self.min) self.min = number;
                if (number > self.max) self.max = number;
            }
            self.count += 1;
            self.sum += number;
        } else {
            self.invalid += 1;
        }
    }

    fn average(self: Stats) f64 {
        return if (self.count == 0) 0 else self.sum / @as(f64, @floatFromInt(self.count));
    }
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        std.debug.print("usage: {s} <file.csv>\n", .{args[0]});
        return;
    }

    const data = try std.fs.cwd().readFileAlloc(allocator, args[1], 100 * 1024 * 1024);

    var rows = std.ArrayList([]const []const u8).init(allocator);
    defer rows.deinit();

    var fields = std.ArrayList([]const u8).init(allocator);
    var field = std.ArrayList(u8).init(allocator);
    var quoted = false;
    var i: usize = 0;

    while (i < data.len) : (i += 1) {
        const c = data[i];

        if (c == '"') {
            if (quoted and i + 1 < data.len and data[i + 1] == '"') {
                try field.append('"');
                i += 1;
            } else {
                quoted = !quoted;
            }
        } else if (!quoted and c == ',') {
            try fields.append(try field.toOwnedSlice());
        } else if (!quoted and (c == '\n' or c == '\r')) {
            try fields.append(try field.toOwnedSlice());
            try rows.append(try fields.toOwnedSlice());

            if (c == '\r' and i + 1 < data.len and data[i + 1] == '\n') {
                i += 1;
            }

            fields = std.ArrayList([]const u8).init(allocator);
            field = std.ArrayList(u8).init(allocator);
        } else {
            try field.append(c);
        }
    }

    if (field.items.len > 0 or fields.items.len > 0) {
        try fields.append(try field.toOwnedSlice());
        try rows.append(try fields.toOwnedSlice());
    }

    if (rows.items.len == 0) return;

    const headers = rows.items[0];
    var stats = try allocator.alloc(Stats, headers.len);
    @memset(stats, Stats{});

    for (rows.items[1..]) |row| {
        for (headers, 0..) |_, column| {
            if (column >= row.len or row[column].len == 0) {
                stats[column].missing += 1;
                continue;
            }

            const value = std.fmt.parseFloat(f64, row[column]) catch {
                stats[column].invalid += 1;
                continue;
            };

            stats[column].add(value);
        }
    }

    var writer = std.io.getStdOut().writer();

    for (headers, 0..) |header, column| {
        const stat = stats[column];

        try writer.print("{s}:\n", .{header});
        try writer.print("  numeric_count: {d}\n", .{stat.count});
        try writer.print("  missing_count: {d}\n", .{stat.missing});
        try writer.print("  invalid_count: {d}\n", .{stat.invalid});

        if (stat.count > 0) {
            try writer.print("  min: {d}\n", .{stat.min});
            try writer.print("  max: {d}\n", .{stat.max});
            try writer.print("  sum: {d}\n", .{stat.sum});
            try writer.print("  average: {d}\n", .{stat.average()});
        }
    }
}
