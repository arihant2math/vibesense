const std = @import("std");

pub const Url = struct {
    scheme: []const u8,
    host: []const u8,
    path: []const u8,
    query: []const u8,
};

pub const ParseError = error{
    InvalidUrl,
};

pub fn parseUrl(input: []const u8) ParseError!Url {
    const scheme_end = std.mem.indexOfScalar(u8, input, ':') orelse {
        return error.InvalidUrl;
    };

    if (scheme_end == 0 or
        scheme_end + 3 > input.len or
        !std.mem.eql(u8, input[scheme_end .. scheme_end + 3], "://"))
    {
        return error.InvalidUrl;
    }

    const scheme = input[0..scheme_end];
    const authority_start = scheme_end + 3;

    var authority_end = input.len;
    for (input[authority_start..], 0..) |character, index| {
        if (character == '/' or character == '?') {
            authority_end = authority_start + index;
            break;
        }
    }

    if (authority_end == authority_start) {
        return error.InvalidUrl;
    }

    const host = input[authority_start..authority_end];

    var path_start = authority_end;
    var query_start: ?usize = null;

    if (path_start < input.len and input[path_start] == '?') {
        query_start = path_start + 1;
    } else {
        for (input[path_start..], 0..) |character, index| {
            if (character == '?') {
                query_start = path_start + index + 1;
                break;
            }
        }
    }

    const path_end = if (query_start) |start| start - 1 else input.len;
    const path = input[path_start..path_end];
    const query = if (query_start) |start| input[start..] else input[input.len..];

    return .{
        .scheme = scheme,
        .host = host,
        .path = path,
        .query = query,
    };
}

test "parses a complete URL" {
    const url = try parseUrl("https://example.com/users/list?page=2");

    try std.testing.expectEqualStrings("https", url.scheme);
    try std.testing.expectEqualStrings("example.com", url.host);
    try std.testing.expectEqualStrings("/users/list", url.path);
    try std.testing.expectEqualStrings("page=2", url.query);
}

test "parses a URL without path or query" {
    const url = try parseUrl("http://example.com");

    try std.testing.expectEqualStrings("http", url.scheme);
    try std.testing.expectEqualStrings("example.com", url.host);
    try std.testing.expectEqualStrings("", url.path);
    try std.testing.expectEqualStrings("", url.query);
}

test "parses a URL with query only" {
    const url = try parseUrl("https://example.com?active=true");

    try std.testing.expectEqualStrings("https", url.scheme);
    try std.testing.expectEqualStrings("example.com", url.host);
    try std.testing.expectEqualStrings("", url.path);
    try std.testing.expectEqualStrings("active=true", url.query);
}

test "rejects invalid URLs" {
    try std.testing.expectError(error.InvalidUrl, parseUrl("example.com/path"));
    try std.testing.expectError(error.InvalidUrl, parseUrl("://example.com"));
    try std.testing.expectError(error.InvalidUrl, parseUrl("https:///path"));
}
