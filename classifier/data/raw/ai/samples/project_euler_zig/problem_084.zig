const std = @import("std");

// Project Euler 84: modal squares for Monopoly with two four-sided dice.
const squares = 40;
const states = squares * 3;

fn addRaw(next: *[states]f64, position: usize, doubles: usize, probability: f64) void {
    next[position * 3 + doubles] += probability;
}

fn addLanding(next: *[states]f64, position: usize, doubles: usize, probability: f64) void {
    if (position == 30) {
        addRaw(next, 10, 0, probability);
    } else if (position == 2 or position == 17 or position == 33) {
        addRaw(next, 0, doubles, probability / 16.0);
        addRaw(next, 10, 0, probability / 16.0);
        addRaw(next, position, doubles, probability * 14.0 / 16.0);
    } else if (position == 7 or position == 22 or position == 36) {
        const next_railway: usize = switch (position) { 7 => 15, 22 => 25, else => 5 };
        const next_utility: usize = if (position == 22) 28 else 12;
        addRaw(next, 0, doubles, probability / 16.0);
        addRaw(next, 10, 0, probability / 16.0);
        addRaw(next, 11, doubles, probability / 16.0);
        addRaw(next, 24, doubles, probability / 16.0);
        addRaw(next, 39, doubles, probability / 16.0);
        addRaw(next, 5, doubles, probability / 16.0);
        addRaw(next, next_railway, doubles, probability * 2.0 / 16.0);
        addRaw(next, next_utility, doubles, probability / 16.0);
        addLanding(next, (position + 37) % squares, doubles, probability / 16.0); // Go back three squares.
        addRaw(next, position, doubles, probability * 6.0 / 16.0);
    } else {
        addRaw(next, position, doubles, probability);
    }
}

pub fn main() !void {
    var probability: [states]f64 = [_]f64{0.0} ** states;
    probability[0] = 1.0;

    for (0..300) |_| {
        var next: [states]f64 = [_]f64{0.0} ** states;
        for (probability, 0..) |state_probability, state| {
            if (state_probability == 0.0) continue;
            const position = state / 3;
            const previous_doubles = state % 3;
            for (1..5) |first_die| {
                for (1..5) |second_die| {
                    const roll_probability = state_probability / 16.0;
                    if (first_die == second_die and previous_doubles == 2) {
                        addRaw(&next, 10, 0, roll_probability);
                    } else {
                        const doubles = if (first_die == second_die) previous_doubles + 1 else 0;
                        addLanding(&next, (position + first_die + second_die) % squares, doubles, roll_probability);
                    }
                }
            }
        }
        probability = next;
    }

    var total: [squares]f64 = [_]f64{0.0} ** squares;
    for (probability, 0..) |state_probability, state| total[state / 3] += state_probability;
    var first: usize = 0;
    for (1..squares) |square| {
        if (total[square] > total[first]) first = square;
    }
    var second: usize = if (first == 0) 1 else 0;
    for (0..squares) |square| {
        if (square != first and total[square] > total[second]) second = square;
    }
    var third: usize = if (first != 0 and second != 0) 0 else 1;
    for (0..squares) |square| {
        if (square != first and square != second and total[square] > total[third]) third = square;
    }

    var buffer: [64]u8 = undefined;
    var writer = std.Io.File.stdout().writer(std.Options.debug_io, &buffer);
    try writer.interface.print("{d:0>2}{d:0>2}{d:0>2}\n", .{ first, second, third });
    try writer.interface.flush();
}
