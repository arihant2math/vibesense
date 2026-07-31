#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

struct Configuration {
    bool verbose = false;
    bool help = false;
    int port = 8080;
    std::string host = "127.0.0.1";
    std::optional<std::string> output;
};

/**
 * Parses command-line arguments into a typed configuration.
 */
class ArgumentParser {
public:
    /**
     * Constructs an argument parser with the specified program name.
     *
     * @param program_name Name displayed in generated usage text.
     */
    explicit ArgumentParser(std::string program_name)
        : program_name_(std::move(program_name)) {}

    /**
     * Parses command-line arguments.
     *
     * @param argc Number of command-line arguments.
     * @param argv Command-line argument values.
     * @return A populated configuration object.
     * @throws std::invalid_argument If an argument is unknown or malformed.
     */
    Configuration parse(int argc, char* argv[]) const {
        Configuration config;

        for (int i = 1; i < argc; ++i) {
            const std::string_view argument(argv[i]);

            if (argument == "--help" || argument == "-h") {
                config.help = true;
            } else if (argument == "--verbose" || argument == "-v") {
                config.verbose = true;
            } else if (argument == "--port" || argument == "-p") {
                config.port = parse_int(require_value(argc, argv, ++i, argument),
                                        "port");
                if (config.port < 1 || config.port > 65535) {
                    throw std::invalid_argument("port must be between 1 and 65535");
                }
            } else if (argument == "--host") {
                config.host = std::string(require_value(argc, argv, ++i, argument));
                if (config.host.empty()) {
                    throw std::invalid_argument("host cannot be empty");
                }
            } else if (argument == "--output" || argument == "-o") {
                config.output =
                    std::string(require_value(argc, argv, ++i, argument));
            } else if (argument.starts_with("--port=")) {
                config.port = parse_int(argument.substr(7), "port");
                if (config.port < 1 || config.port > 65535) {
                    throw std::invalid_argument("port must be between 1 and 65535");
                }
            } else if (argument.starts_with("--host=")) {
                config.host = std::string(argument.substr(7));
            } else if (argument.starts_with("--output=")) {
                config.output = std::string(argument.substr(9));
            } else {
                throw std::invalid_argument(
                    "unknown argument: " + std::string(argument));
            }
        }

        return config;
    }

    /**
     * Returns formatted command-line usage documentation.
     *
     * @return Usage text for the configured program name.
     */
    std::string usage() const {
        return "Usage: " + program_name_ +
               " [options]\n"
               "\n"
               "Options:\n"
               "  -h, --help           Show this help message\n"
               "  -v, --verbose        Enable verbose output\n"
               "  -p, --port PORT      Listen on port 1-65535\n"
               "      --host HOST      Listen on host address\n"
               "  -o, --output FILE    Write output to FILE\n";
    }

private:
    std::string program_name_;

    static std::string_view require_value(int argc,
                                          char* argv[],
                                          int index,
                                          std::string_view option) {
        if (index >= argc) {
            throw std::invalid_argument("missing value for " +
                                        std::string(option));
        }
        return argv[index];
    }

    static int parse_int(std::string_view value, std::string_view option) {
        if (value.empty()) {
            throw std::invalid_argument("missing value for " +
                                        std::string(option));
        }

        std::string text(value);
        std::size_t position = 0;

        try {
            const int result = std::stoi(text, &position);
            if (position != text.size()) {
                throw std::invalid_argument("");
            }
            return result;
        } catch (const std::exception&) {
            throw std::invalid_argument("invalid integer for " +
                                        std::string(option) + ": " + text);
        }
    }
};

/**
 * Runs the command-line application.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument values.
 * @return Zero on success, nonzero on failure.
 */
int main(int argc, char* argv[]) {
    ArgumentParser parser(argc > 0 ? argv[0] : "application");

    try {
        const Configuration config = parser.parse(argc, argv);

        if (config.help) {
            std::cout << parser.usage();
            return 0;
        }

        if (config.verbose) {
            std::cout << "Host: " << config.host << '\n'
                      << "Port: " << config.port << '\n'
                      << "Output: "
                      << (config.output ? *config.output : "<stdout>") << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n"
                  << parser.usage();
        return 1;
    }
}
