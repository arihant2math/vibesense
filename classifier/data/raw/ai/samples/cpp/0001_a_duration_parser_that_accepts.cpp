#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

struct DurationParseResult {
    bool success;
    std::uint64_t milliseconds;
    std::string error;

    static DurationParseResult ok(std::uint64_t value) {
        return {true, value, {}};
    }

    static DurationParseResult fail(std::string message) {
        return {false, 0, std::move(message)};
    }
};

DurationParseResult parse_duration(std::string_view input) {
    if (input.empty()) {
        return DurationParseResult::fail("duration must not be empty");
    }

    constexpr std::uint64_t max_value =
        std::numeric_limits<std::uint64_t>::max();

    std::uint64_t total = 0;
    std::size_t position = 0;
    int previous_unit_rank = 100;
    bool parsed_component = false;

    while (position < input.size()) {
        const std::size_t number_start = position;

        if (input[position] < '0' || input[position] > '9') {
            return DurationParseResult::fail(
                "expected a number at position " + std::to_string(position));
        }

        std::uint64_t number = 0;
        while (position < input.size() &&
               input[position] >= '0' && input[position] <= '9') {
            const std::uint64_t digit =
                static_cast<std::uint64_t>(input[position] - '0');

            if (number > (max_value - digit) / 10) {
                return DurationParseResult::fail(
                    "numeric value overflows at position " +
                    std::to_string(number_start));
            }

            number = number * 10 + digit;
            ++position;
        }

        if (position == input.size()) {
            return DurationParseResult::fail(
                "missing unit after number at position " +
                std::to_string(number_start));
        }

        std::uint64_t unit_multiplier = 0;
        int unit_rank = 0;

        switch (input[position]) {
            case 'd':
                unit_multiplier = 86'400'000;
                unit_rank = 4;
                ++position;
                break;
            case 'h':
                unit_multiplier = 3'600'000;
                unit_rank = 3;
                ++position;
                break;
            case 'm':
                unit_multiplier = 60'000;
                unit_rank = 2;
                ++position;
                break;
            case 's':
                unit_multiplier = 1'000;
                unit_rank = 1;
                ++position;
                break;
            case 'u':
                if (position + 1 >= input.size() || input[position + 1] != 's') {
                    return DurationParseResult::fail(
                        "invalid unit at position " + std::to_string(position));
                }
                unit_multiplier = 0;
                unit_rank = 0;
                position += 2;
                break;
            case 'm':
                break;
            default:
                return DurationParseResult::fail(
                    "invalid unit at position " + std::to_string(position));
        }

        if (unit_rank >= previous_unit_rank) {
            return DurationParseResult::fail(
                "units must appear in descending order without duplicates");
        }

        if (unit_multiplier != 0 &&
            number > max_value / unit_multiplier) {
            return DurationParseResult::fail(
                "duration component overflows at position " +
                std::to_string(number_start));
        }

        const std::uint64_t component = number * unit_multiplier;

        if (total > max_value - component) {
            return DurationParseResult::fail("total duration overflows");
        }

        total += component;
        previous_unit_rank = unit_rank;
        parsed_component = true;
    }

    if (!parsed_component) {
        return DurationParseResult::fail("duration contains no components");
    }

    return DurationParseResult::ok(total);
}
