#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <system_error>

namespace fs = std::filesystem;

class SHA256 {
public:
    SHA256() { reset(); }

    void update(const uint8_t* data, std::size_t length) {
        bitCount_ += static_cast<uint64_t>(length) * 8;
        while (length > 0) {
            const std::size_t available = 64 - bufferSize_;
            const std::size_t chunk = length < available ? length : available;
            std::copy(data, data + chunk, buffer_.begin() + bufferSize_);
            bufferSize_ += chunk;
            data += chunk;
            length -= chunk;

            if (bufferSize_ == 64) {
                transform(buffer_.data());
                bufferSize_ = 0;
            }
        }
    }

    std::string final() {
        const uint64_t originalBitCount = bitCount_;

        buffer_[bufferSize_++] = 0x80;
        if (bufferSize_ > 56) {
            while (bufferSize_ < 64) buffer_[bufferSize_++] = 0;
            transform(buffer_.data());
            bufferSize_ = 0;
        }

        while (bufferSize_ < 56) buffer_[bufferSize_++] = 0;
        for (int i = 7; i >= 0; --i)
            buffer_[bufferSize_++] =
                static_cast<uint8_t>((originalBitCount >> (i * 8)) & 0xff);

        transform(buffer_.data());

        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (uint32_t word : state_)
            out << std::setw(8) << word;

        reset();
        return out.str();
    }

private:
    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,
        0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,
        0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,
        0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
        0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,
        0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
        0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,
        0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,
        0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,
        0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,
        0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
        0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    static uint32_t rotr(uint32_t x, unsigned n) {
        return (x >> n) | (x << (32 - n));
    }

    void reset() {
        state_ = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        buffer_.fill(0);
        bufferSize_ = 0;
        bitCount_ = 0;
    }

    void transform(const uint8_t* block) {
        std::array<uint32_t, 64> w{};

        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(block[i * 4 + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^
                                rotr(w[i - 15], 18) ^
                                (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^
                                rotr(w[i - 2], 19) ^
                                (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

        for (int i = 0; i < 64; ++i) {
            const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t temp1 = h + s1 + ch + K[i] + w[i];
            const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<uint32_t, 8> state_{};
    std::array<uint8_t, 64> buffer_{};
    std::size_t bufferSize_ = 0;
    uint64_t bitCount_ = 0;
};

std::string hashFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open file: " + path.string());

    SHA256 hash;
    std::array<char, 1024 * 1024> buffer{};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0) {
            hash.update(
                reinterpret_cast<const uint8_t*>(buffer.data()),
                static_cast<std::size_t>(count)
            );
        }
    }

    if (!input.eof())
        throw std::runtime_error("error reading file: " + path.string());

    return hash.final();
}

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " <directory>\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            printUsage(argv[0]);
            return 2;
        }

        const fs::path root = fs::absolute(fs::path(argv[1]));

        std::error_code ec;
        if (!fs::exists(root, ec) || ec) {
            throw std::runtime_error("directory does not exist: " + root.string());
        }
        if (!fs::is_directory(root, ec) || ec) {
            throw std::runtime_error("path is not a directory: " + root.string());
        }

        std::map<std::uintmax_t, std::vector<fs::path>> bySize;
        fs::directory_options options = fs::directory_options::skip_permission_denied;

        fs::recursive_directory_iterator it(root, options, ec);
        fs::recursive_directory_iterator end;

        if (ec)
            throw std::runtime_error("cannot traverse directory: " + ec.message());

        for (; it != end; it.increment(ec)) {
            if (ec) {
                std::cerr << "Warning: traversal error: " << ec.message() << '\n';
                ec.clear();
                continue;
            }

            const fs::path path = it->path();
            std::error_code fileEc;
            if (!it->is_regular_file(fileEc)) {
                if (fileEc)
                    std::cerr << "Warning: cannot inspect " << path << ": "
                              << fileEc.message() << '\n';
                continue;
            }

            const auto size = it->file_size(fileEc);
            if (fileEc) {
                std::cerr << "Warning: cannot determine size of " << path << ": "
                          << fileEc.message() << '\n';
                continue;
            }

            bySize[size].push_back(path);
        }

        std::map<std::string, std::vector<fs::path>> groups;

        for (const auto& [size, files] : bySize) {
            if (files.size() < 2)
                continue;

            for (const auto& file : files) {
                try {
                    groups[hashFile(file)].push_back(file);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: " << e.what() << '\n';
                }
            }
        }

        bool found = false;
        for (const auto& [hash, files] : groups) {
            if (files.size() < 2)
                continue;

            found = true;
            std::cout << hash << '\n';
            for (const auto& file : files)
                std::cout << "  " << file.string() << '\n';
        }

        return found ? 0 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
