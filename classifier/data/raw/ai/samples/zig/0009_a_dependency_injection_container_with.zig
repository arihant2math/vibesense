const std = @import("std");

pub const ContainerError = error{
    ServiceAlreadyRegistered,
    ServiceNotRegistered,
    CircularDependency,
    CapacityOverflow,
};

const State = enum {
    uninitialized,
    initializing,
    initialized,
};

const ServiceEntry = struct {
    name: []const u8,
    state: State = .uninitialized,
    instance: ?*anyopaque = null,
    create: *const fn (*Container) anyerror!*anyopaque,
    destroy: *const fn (std.mem.Allocator, *anyopaque) void,
};

pub const Container = struct {
    allocator: std.mem.Allocator,
    entries: []ServiceEntry,
    len: usize,

    pub fn init(allocator: std.mem.Allocator) Container {
        return .{
            .allocator = allocator,
            .entries = &.{},
            .len = 0,
        };
    }

    pub fn deinit(self: *Container) void {
        var i: usize = self.len;
        while (i > 0) {
            i -= 1;
            const entry = &self.entries[i];
            if (entry.instance) |instance| {
                entry.destroy(self.allocator, instance);
            }
        }

        if (self.entries.len != 0) {
            self.allocator.free(self.entries);
        }

        self.entries = &.{};
        self.len = 0;
    }

    pub fn registerSingleton(
        self: *Container,
        comptime T: type,
        comptime factory: *const fn (*Container) anyerror!*T,
    ) !void {
        const name = @typeName(T);

        if (self.findIndex(name) != null) {
            return error.ServiceAlreadyRegistered;
        }

        try self.ensureCapacity(1);

        self.entries[self.len] = .{
            .name = name,
            .create = makeCreator(T, factory),
            .destroy = makeDestroyer(T),
        };

        self.len += 1;
    }

    pub fn resolve(self: *Container, comptime T: type) !*T {
        const name = @typeName(T);
        const index = self.findIndex(name) orelse return error.ServiceNotRegistered;
        const entry = &self.entries[index];

        switch (entry.state) {
            .initialized => {
                return @ptrCast(@alignCast(entry.instance.?));
            },
            .initializing => return error.CircularDependency,
            .uninitialized => {},
        }

        entry.state = .initializing;
        errdefer entry.state = .uninitialized;

        const instance = try entry.create(self);
        entry.instance = instance;
        entry.state = .initialized;

        return @ptrCast(@alignCast(instance));
    }

    fn findIndex(self: *const Container, name: []const u8) ?usize {
        var i: usize = 0;
        while (i < self.len) : (i += 1) {
            if (std.mem.eql(u8, self.entries[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    fn ensureCapacity(self: *Container, additional: usize) !void {
        const required = std.math.add(usize, self.len, additional) catch {
            return error.CapacityOverflow;
        };

        if (required <= self.entries.len) {
            return;
        }

        var capacity = if (self.entries.len == 0) @as(usize, 4) else self.entries.len;
        while (capacity < required) {
            const doubled = std.math.mul(usize, capacity, 2) catch {
                return error.CapacityOverflow;
            };
            capacity = doubled;
        }

        const bytes = std.math.mul(usize, capacity, @sizeOf(ServiceEntry)) catch {
            return error.CapacityOverflow;
        };

        const new_entries = try self.allocator.alloc(ServiceEntry, capacity);
        if (self.len != 0) {
            @memcpy(new_entries[0..self.len], self.entries[0..self.len]);
            self.allocator.free(self.entries);
        }

        _ = bytes;
        self.entries = new_entries;
    }
};

fn makeCreator(
    comptime T: type,
    comptime factory: *const fn (*Container) anyerror!*T,
) *const fn (*Container) anyerror!*anyopaque {
    return struct {
        fn create(container: *Container) anyerror!*anyopaque {
            const value = try factory(container);
            return @ptrCast(value);
        }
    }.create;
}

fn makeDestroyer(
    comptime T: type,
) *const fn (std.mem.Allocator, *anyopaque) void {
    return struct {
        fn destroy(allocator: std.mem.Allocator, raw: *anyopaque) void {
            const value: *T = @ptrCast(@alignCast(raw));
            allocator.destroy(value);
        }
    }.destroy;
}
