#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>

class WriteAheadLog {
public:
    using Record = std::string;
    using ReplayHandler = std::function<void(const Record&)>;

    explicit WriteAheadLog(const std::filesystem::path& path)
        : path_(path) {}

    void append(const Record& record) {
        if (record.size() > std::numeric_limits<std::uint64_t>::max()) {
            throw std::length_error("record is too large");
        }

        const auto checksum = hash(record);
        std::lock_guard<std::mutex> lock(mutex_);

        std::ofstream out(path_, std::ios::binary | std::ios::app);
        if (!out) {
            throw std::runtime_error("failed to open WAL for append");
        }

        write_u64(out, static_cast<std::uint64_t>(record.size()));
        write_u64(out, checksum);
        out.write(record.data(), static_cast<std::streamsize>(record.size()));
        out.flush();

        if (!out) {
            throw std::runtime_error("failed to append WAL record");
        }
    }

    void replay(const ReplayHandler& handler) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ifstream in(path_, std::ios::binary);
        if (!in) {
            if (std::filesystem::exists(path_)) {
                throw std::runtime_error("failed to open WAL for replay");
            }
            return;
        }

        constexpr std::uint64_t max_record_size = 1ULL << 32;

        while (true) {
            std::uint64_t size = 0;
            std::uint64_t expected_checksum = 0;

            const auto header_status = read_u64(in, size);
            if (header_status == ReadStatus::End) {
                return;
            }
            if (header_status == ReadStatus::Partial ||
                read_u64(in, expected_checksum) != ReadStatus::Complete) {
                return;
            }

            if (size > max_record_size ||
                size > static_cast<std::uint64_t>(
                            std::numeric_limits<std::size_t>::max())) {
                throw std::runtime_error("WAL record is too large");
            }

            Record record(static_cast<std::size_t>(size), '\0');
            in.read(record.data(), static_cast<std::streamsize>(size));

            if (in.gcount() != static_cast<std::streamsize>(size)) {
                return;
            }

            if (hash(record) != expected_checksum) {
                throw std::runtime_error("WAL checksum mismatch");
            }

            handler(record);
        }
    }

private:
    enum class ReadStatus {
        Complete,
        End,
        Partial
    };

    static constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
    static constexpr std::uint64_t fnv_prime = 1099511628211ULL;

    static std::uint64_t hash(const std::string& data) {
        std::uint64_t value = fnv_offset;
        for (unsigned char byte : data) {
            value ^= byte;
            value *= fnv_prime;
        }
        return value;
    }

    static void write_u64(std::ofstream& out, std::uint64_t value) {
        char bytes[8];
        for (int i = 0; i < 8; ++i) {
            bytes[i] = static_cast<char>(value & 0xff);
            value >>= 8;
        }
        out.write(bytes, sizeof(bytes));
    }

    static ReadStatus read_u64(std::ifstream& in, std::uint64_t& value) {
        char bytes[8];
        in.read(bytes, sizeof(bytes));

        const auto count = in.gcount();
        if (count == 0) {
            return ReadStatus::End;
        }
        if (count != static_cast<std::streamsize>(sizeof(bytes))) {
            return ReadStatus::Partial;
        }

        value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<std::uint64_t>(
                         static_cast<unsigned char>(bytes[i]))
                     << (i * 8);
        }

        return ReadStatus::Complete;
    }

    std::filesystem::path path_;
    mutable std::mutex mutex_;
};
