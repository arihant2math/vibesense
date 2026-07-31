#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path)
        : fd_(::open(path.c_str(), O_CREAT | O_RDWR | O_APPEND, 0644)) {
        if (fd_ < 0) throw std::runtime_error("failed to open WAL");
    }

    ~WriteAheadLog() {
        if (fd_ >= 0) ::close(fd_);
    }

    WriteAheadLog(const WriteAheadLog&) = delete;
    WriteAheadLog& operator=(const WriteAheadLog&) = delete;

    void append(const std::string& data) {
        if (data.size() > UINT32_MAX)
            throw std::runtime_error("record too large");

        Header header{Magic, static_cast<uint32_t>(data.size())};
        writeAll(&header, sizeof(header));
        writeAll(data.data(), data.size());

        if (::fsync(fd_) != 0)
            throw std::runtime_error("failed to flush WAL");
    }

    void replay(const std::function<void(const std::string&)>& callback) {
        if (::lseek(fd_, 0, SEEK_SET) < 0)
            throw std::runtime_error("failed to seek WAL");

        while (true) {
            Header header;
            ssize_t n = ::read(fd_, &header, sizeof(header));

            if (n == 0) break;
            if (n != static_cast<ssize_t>(sizeof(header))) break;
            if (header.magic != Magic) break;
            if (header.length > MaxRecordSize) break;

            std::string record(header.length, '\0');
            if (!readAll(record.data(), record.size())) break;

            callback(record);
        }

        if (::lseek(fd_, 0, SEEK_END) < 0)
            throw std::runtime_error("failed to restore WAL position");
    }

private:
    static constexpr uint32_t Magic = 0x57414C31;
    static constexpr uint32_t MaxRecordSize = 256 * 1024 * 1024;

    struct Header {
        uint32_t magic;
        uint32_t length;
    };

    int fd_;

    void writeAll(const void* buffer, size_t size) {
        const char* ptr = static_cast<const char*>(buffer);

        while (size > 0) {
            ssize_t n = ::write(fd_, ptr, size);
            if (n <= 0) throw std::runtime_error("failed to write WAL");
            ptr += n;
            size -= static_cast<size_t>(n);
        }
    }

    bool readAll(void* buffer, size_t size) {
        char* ptr = static_cast<char*>(buffer);

        while (size > 0) {
            ssize_t n = ::read(fd_, ptr, size);
            if (n <= 0) return false;
            ptr += n;
            size -= static_cast<size_t>(n);
        }

        return true;
    }
};

int main() {
    WriteAheadLog wal("events.wal");

    wal.append("first event");
    wal.append("second event");

    wal.replay([](const std::string& record) {
        // Apply record to application state.
    });

    return 0;
}
