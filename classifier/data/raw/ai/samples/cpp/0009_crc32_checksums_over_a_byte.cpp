#include <cstddef>
#include <cstdint>
#include <span>

namespace crc32 {

constexpr std::uint32_t polynomial = 0xEDB88320u;

constexpr std::uint32_t update(std::uint32_t crc, std::uint8_t byte) noexcept {
    crc ^= byte;

    for (int bit = 0; bit < 8; ++bit) {
        crc = (crc >> 1) ^ ((crc & 1u) ? polynomial : 0u);
    }

    return crc;
}

constexpr std::uint32_t compute(std::span<const std::uint8_t> data) noexcept {
    std::uint32_t crc = 0xFFFFFFFFu;

    for (const auto byte : data) {
        crc = update(crc, byte);
    }

    return crc ^ 0xFFFFFFFFu;
}

} // namespace crc32
