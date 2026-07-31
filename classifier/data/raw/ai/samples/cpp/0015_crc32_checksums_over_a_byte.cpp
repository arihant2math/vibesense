#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

std::uint32_t crc32(const std::uint8_t* data, std::size_t length) {
    std::uint32_t crc = 0xFFFFFFFFu;

    for (std::size_t i = 0; i < length; ++i) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1u)));
        }
    }

    return crc ^ 0xFFFFFFFFu;
}

int main() {
    const std::vector<std::uint8_t> buffer(
        std::istreambuf_iterator<char>(std::cin),
        std::istreambuf_iterator<char>());

    const std::uint32_t checksum = crc32(buffer.data(), buffer.size());

    std::cout << std::hex << checksum << '\n';
    return 0;
}
