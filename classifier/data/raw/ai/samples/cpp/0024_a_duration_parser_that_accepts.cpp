#include <cctype>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

std::chrono::milliseconds parse_duration(std::string_view input) {
    using namespace std::chrono;

    if (input.empty()) {
        throw std::invalid_argument("empty duration");
    }

    milliseconds total{0};
    std::size_t pos = 0;

    while (pos < input.size()) {
        if (!std::isdigit(static_cast<unsigned char>(input[pos]))) {
            throw std::invalid_argument("expected a number");
        }

        std::uint64_t value = 0;
        while (pos < input.size() &&
               std::isdigit(static_cast<unsigned char>(input[pos]))) {
            const auto digit = static_cast<unsigned>(input[pos] - '0');
            if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
                throw std::out_of_range("duration value overflow");
            }
            value = value * 10 + digit;
            ++pos;
        }

        if (pos >= input.size()) {
            throw std::invalid_argument("missing duration unit");
        }

        milliseconds part{0};
        if (input.substr(pos, 2) == "ms") {
            if (value > static_cast<std::uint64_t>(milliseconds::max().count())) {
                throw std::out_of_range("duration overflow");
            }
            part = milliseconds{static_cast<milliseconds::rep>(value)};
            pos += 2;
        } else {
            char unit = input[pos++];
            switch (unit) {
                case 's':
                    part = duration_cast<milliseconds>(seconds{value});
                    break;
                case 'm':
                    part = duration_cast<milliseconds>(minutes{value});
                    break;
                case 'h':
                    part = duration_cast<milliseconds>(hours{value});
                    break;
                case 'd':
                    part = duration_cast<milliseconds>(hours{24 * value});
                    break;
                default:
                    throw std::invalid_argument("unknown duration unit");
            }
        }

        if (total.count() > milliseconds::max().count() - part.count()) {
            throw std::out_of_range("duration overflow");
        }
        total += part;
    }

    return total;
}

int main() {
    std::string input;
    std::getline(std::cin, input);

    try {
        std::cout << parse_duration(input).count() << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
