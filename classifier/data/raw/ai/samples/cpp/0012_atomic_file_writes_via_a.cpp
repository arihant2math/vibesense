#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) noexcept : fd_(fd) {}
    ~FileDescriptor() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    int get() const noexcept { return fd_; }

    int release() noexcept {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }

private:
    int fd_;
};

static void throw_errno(const std::string& operation,
                        const fs::path& path = {}) {
    const int error = errno;
    std::string message = operation;
    if (!path.empty()) {
        message += " '" + path.string() + "'";
    }
    message += ": ";
    message += std::strerror(error);
    throw std::system_error(error, std::generic_category(), message);
}

static void write_all(int fd, const char* data, std::size_t size,
                      const fs::path& path) {
    std::size_t offset = 0;

    while (offset < size) {
        const ssize_t written =
            ::write(fd, data + offset, size - offset);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw_errno("write failed", path);
        }

        if (written == 0) {
            throw std::runtime_error(
                "write failed '" + path.string() + "': zero bytes written");
        }

        offset += static_cast<std::size_t>(written);
    }
}

void atomic_write(const fs::path& target, const std::string& data) {
    if (target.empty()) {
        throw std::invalid_argument("target path must not be empty");
    }

    if (target.filename().empty() ||
        target.filename() == "." ||
        target.filename() == "..") {
        throw std::invalid_argument("target path must name a file");
    }

    if (target.filename().string().find('\0') != std::string::npos) {
        throw std::invalid_argument("target path contains a null character");
    }

    std::error_code ec;
    const fs::path absolute_target = fs::absolute(target, ec);
    if (ec) {
        throw std::system_error(ec, "cannot resolve target path");
    }

    const fs::path parent = absolute_target.parent_path();
    if (parent.empty()) {
        throw std::invalid_argument("target path has no parent directory");
    }

    if (!fs::exists(parent, ec) || ec) {
        if (ec) {
            throw std::system_error(ec, "cannot inspect parent directory");
        }
        throw std::runtime_error("parent directory does not exist: " +
                                 parent.string());
    }

    if (!fs::is_directory(parent, ec) || ec) {
        if (ec) {
            throw std::system_error(ec, "cannot inspect parent directory");
        }
        throw std::runtime_error("parent path is not a directory: " +
                                 parent.string());
    }

    if (fs::exists(absolute_target, ec) && !ec &&
        fs::is_directory(absolute_target, ec)) {
        throw std::runtime_error("target path is a directory: " +
                                 absolute_target.string());
    }

    if (ec) {
        throw std::system_error(ec, "cannot inspect target path");
    }

    std::string template_path =
        (parent / (absolute_target.filename().string() + ".tmp.XXXXXX"))
            .string();

    std::vector<char> mutable_template(template_path.begin(),
                                        template_path.end());
    mutable_template.push_back('\0');

    const int raw_fd = ::mkstemp(mutable_template.data());
    if (raw_fd < 0) {
        throw_errno("cannot create temporary file", parent);
    }

    FileDescriptor temp_fd(raw_fd);
    const fs::path temporary_path(mutable_template.data());

    try {
        write_all(temp_fd.get(), data.data(), data.size(), temporary_path);

        if (::fsync(temp_fd.get()) < 0) {
            throw_errno("fsync failed", temporary_path);
        }

        if (::fchmod(temp_fd.get(), S_IRUSR | S_IWUSR) < 0) {
            throw_errno("cannot set temporary file permissions",
                        temporary_path);
        }

        if (::close(temp_fd.release()) < 0) {
            throw_errno("close failed", temporary_path);
        }

        if (::rename(temporary_path.c_str(), absolute_target.c_str()) < 0) {
            throw_errno("atomic rename failed", absolute_target);
        }

        const int directory_fd =
            ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (directory_fd < 0) {
            throw_errno("cannot open parent directory for syncing", parent);
        }

        FileDescriptor directory(directory_fd);
        if (::fsync(directory.get()) < 0) {
            throw_errno("directory fsync failed", parent);
        }
    } catch (...) {
        std::error_code cleanup_error;
        fs::remove(temporary_path, cleanup_error);
        throw;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <target> <content>\n";
            return 2;
        }

        atomic_write(fs::path(argv[1]), argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
