#include <cstddef>
#include <stdexcept>
#include <string>

namespace base64 {

namespace detail {

constexpr char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline unsigned char decode_value(char c) {
    if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A');
    if (c >= 'a' && c <= 'z') return static_cast<unsigned char>(c - 'a' + 26);
    if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0' + 52);
    if (c == '+') return 62;
    if (c == '/') return 63;
    throw std::invalid_argument("Invalid Base64 character");
}

} // namespace detail

/**
 * Encodes binary data as a Base64 string.
 *
 * @param data The binary input data.
 * @return The Base64-encoded representation of the input.
 */
std::string encode(const std::string& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    for (std::size_t i = 0; i < data.size(); i += 3) {
        const unsigned char byte1 =
            static_cast<unsigned char>(data[i]);
        const bool has_byte2 = i + 1 < data.size();
        const bool has_byte3 = i + 2 < data.size();

        const unsigned char byte2 =
            has_byte2 ? static_cast<unsigned char>(data[i + 1]) : 0;
        const unsigned char byte3 =
            has_byte3 ? static_cast<unsigned char>(data[i + 2]) : 0;

        result += detail::alphabet[(byte1 >> 2) & 0x3F];
        result += detail::alphabet[((byte1 & 0x03) << 4) | (byte2 >> 4)];
        result += has_byte2
            ? detail::alphabet[((byte2 & 0x0F) << 2) | (byte3 >> 6)]
            : '=';
        result += has_byte3
            ? detail::alphabet[byte3 & 0x3F]
            : '=';
    }

    return result;
}

/**
 * Decodes a Base64 string into binary data.
 *
 * @param encoded The Base64-encoded input string.
 * @return The decoded binary data.
 * @throws std::invalid_argument If the input is malformed.
 */
std::string decode(const std::string& encoded) {
    if (encoded.size() % 4 != 0) {
        throw std::invalid_argument("Invalid Base64 length");
    }

    std::string result;
    result.reserve((encoded.size() / 4) * 3);

    for (std::size_t i = 0; i < encoded.size(); i += 4) {
        const char c1 = encoded[i];
        const char c2 = encoded[i + 1];
        const char c3 = encoded[i + 2];
        const char c4 = encoded[i + 3];

        if (c1 == '=' || c2 == '=') {
            throw std::invalid_argument("Invalid Base64 padding");
        }

        const unsigned char v1 = detail::decode_value(c1);
        const unsigned char v2 = detail::decode_value(c2);

        result.push_back(static_cast<char>((v1 << 2) | (v2 >> 4)));

        if (c3 == '=') {
            if (c4 != '=' || i + 4 != encoded.size() || (v2 & 0x0F) != 0) {
                throw std::invalid_argument("Invalid Base64 padding");
            }
            continue;
        }

        const unsigned char v3 = detail::decode_value(c3);
        result.push_back(static_cast<char>((v2 << 4) | (v3 >> 2)));

        if (c4 == '=') {
            if (i + 4 != encoded.size() || (v3 & 0x03) != 0) {
                throw std::invalid_argument("Invalid Base64 padding");
            }
            continue;
        }

        const unsigned char v4 = detail::decode_value(c4);
        result.push_back(static_cast<char>((v3 << 6) | v4));
    }

    return result;
}

} // namespace base64

