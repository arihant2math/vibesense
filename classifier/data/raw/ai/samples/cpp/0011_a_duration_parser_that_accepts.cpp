#include <cctype>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

std::chrono::milliseconds parseDuration(const std::string& input) {
    if (input.empty()) {
        throw std::invalid_argument("Duration cannot be empty");
    }

    std::chrono::milliseconds total{0};
    std::size_t i = 0;

    while (i < input.size()) {
        if (!std::isdigit(static_cast<unsigned char>(input[i]))) {
            throw std::invalid_argument("Expected a number");
        }

        long long value = 0;
        while (i < input.size() &&
               std::isdigit(static_cast<unsigned char>(input[i]))) {
            value = value * 10 + (input[i] - '0');
            ++i;
        }

        if (i >= input.size()) {
            throw std::invalid_argument("Missing time unit");
        }

        long long multiplier;
        switch (input[i]) {
            case 's':
                multiplier = 1000LL;
                break;
            case 'm':
                multiplier = 60LL * 1000;
                break;
            case 'h':
                multiplier = 60LL * 60 * 1000;
                break;
            case 'd':
                multiplier = 24LL * 60 * 60 * 1000;
                break;
            default:
                throw std::invalid_argument("Unknown time unit");
        }

        if (value > std::chrono::milliseconds::max().count() / multiplier) {
            throw std::out_of_range("Duration is too large");
        }

        total += std::chrono::milliseconds(value * multiplier);
        ++i;
    }

    return total;
}

int main() {
    std::string input;
    std::cin >> input;

    try {
        std::cout << parseDuration(input).count() << " ms\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
