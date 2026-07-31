#include <cstdint>
#include <stdexcept>
#include <vector>

class XorRotatingCipher {
public:
    explicit XorRotatingCipher(std::vector<std::uint8_t> key)
        : key_(std::move(key)) {
        if (key_.empty()) {
            throw std::invalid_argument("Key must not be empty");
        }
    }

    std::vector<std::uint8_t> encrypt(
        const std::vector<std::uint8_t>& plaintext) const {
        return transform(plaintext);
    }

    std::vector<std::uint8_t> decrypt(
        const std::vector<std::uint8_t>& ciphertext) const {
        return transform(ciphertext);
    }

private:
    std::vector<std::uint8_t> key_;

    static std::uint8_t rotateLeft(std::uint8_t value, unsigned bits) {
        bits %= 8;
        return static_cast<std::uint8_t>(
            (value << bits) | (value >> (8 - bits)));
    }

    std::vector<std::uint8_t> transform(
        const std::vector<std::uint8_t>& input) const {
        std::vector<std::uint8_t> output;
        output.reserve(input.size());

        std::vector<std::uint8_t> rotatingKey = key_;

        for (std::size_t i = 0; i < input.size(); ++i) {
            const std::size_t keyIndex = i % rotatingKey.size();
            const unsigned rotation = static_cast<unsigned>(i % 8);

            const std::uint8_t keystream =
                rotateLeft(rotatingKey[keyIndex], rotation);

            output.push_back(input[i] ^ keystream);

            rotatingKey[keyIndex] =
                rotateLeft(static_cast<std::uint8_t>(
                               rotatingKey[keyIndex] + 0x3D),
                           1);
        }

        return output;
    }
};
