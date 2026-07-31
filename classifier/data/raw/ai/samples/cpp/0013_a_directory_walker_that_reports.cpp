#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

namespace fs = std::filesystem;

std::map<std::string, std::uintmax_t>
reportFileSizesByExtension(const fs::path& root) {
    std::map<std::string, std::uintmax_t> sizes;

    std::error_code ec;
    fs::recursive_directory_iterator it(
        root,
        fs::directory_options::skip_permission_denied,
        ec
    );
    fs::recursive_directory_iterator end;

    while (it != end) {
        if (ec) {
            ec.clear();
            it.increment(ec);
            continue;
        }

        const fs::directory_entry& entry = *it;

        if (entry.is_regular_file(ec)) {
            std::string extension = entry.path().extension().string();
            if (extension.empty()) {
                extension = "[no extension]";
            }

            std::uintmax_t size = entry.file_size(ec);
            if (!ec) {
                sizes[extension] += size;
            } else {
                ec.clear();
            }
        }

        it.increment(ec);
    }

    return sizes;
}

int main(int argc, char* argv[]) {
    const fs::path root = argc > 1 ? argv[1] : fs::current_path();

    if (!fs::exists(root)) {
        std::cerr << "Directory does not exist: " << root << '\n';
        return 1;
    }

    if (!fs::is_directory(root)) {
        std::cerr << "Path is not a directory: " << root << '\n';
        return 1;
    }

    const auto sizes = reportFileSizesByExtension(root);

    std::cout << "File sizes by extension in: " << root << "\n\n";

    for (const auto& [extension, size] : sizes) {
        std::cout << std::left << std::setw(18)
                  << extension << std::right
                  << std::setw(15) << size << " bytes\n";
    }

    return 0;
}
