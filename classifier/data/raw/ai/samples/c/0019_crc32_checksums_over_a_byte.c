#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int32_t)(crc & 1u)));
        }
    }

    return ~crc;
}

int main(void) {
    const uint8_t data[] = "Hello, CRC32!";
    size_t length = sizeof(data) - 1;
    uint32_t checksum = crc32(data, length);

    printf("CRC32: %08X\n", checksum);
    return 0;
}
