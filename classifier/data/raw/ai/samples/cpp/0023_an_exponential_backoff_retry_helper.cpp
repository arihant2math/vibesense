#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace retry {

/// Configures exponential backoff retry behavior.
struct BackoffOptions {
    /// Maximum number of attempts, including the initial attempt.
    std::size_t max_attempts = 3;

    /// Initial delay before the first retry.
    std::chrono::milliseconds initial_delay{100};

    /// Maximum delay between attempts.
    std::chrono::milliseconds max_delay{30'000};

    /// Multiplier applied to the delay after each failed attempt.
    double multiplier = 2.0;

    /// Fractional jitter range applied symmetrically around the calculated delay.
    double jitter = 0.2;
};

/// Calculates a jittered exponential backoff delay.
inline std::chrono::milliseconds calculate_delay(
    std::size_t retry_number,
    const BackoffOptions& options,
    std::mt19937_64& random_engine) {
    if (options.initial_delay.count() < 0 ||
        options.max_delay.count() < 0 ||
        options.max_delay < options.initial_delay ||
        options.multiplier < 1.0 ||
        options.jitter < 0.0 ||
        options.jitter > 1.0) {
        throw std::invalid_argument("invalid backoff options");
    }

    const double exponential_delay =
        static_cast<double>(options.initial_delay.count()) *
        std::pow(options.multiplier, static_cast<double>(retry_number));

    const double capped_delay = std::min(
        exponential_delay,
        static_cast<double>(options.max_delay.count()));

    const double lower_bound = capped_delay * (1.0 - options.jitter);
    const double upper_bound = capped_delay * (1.0 + options.jitter);

    std::uniform_real_distribution<double> distribution(lower_bound, upper_bound);
    return std::chrono::milliseconds{
        static_cast<long long>(distribution(random_engine))};
}

/// Executes an operation and retries failures using exponential backoff with jitter.
template <typename Operation, typename RetryPredicate>
decltype(auto) retry(
    Operation&& operation,
    const BackoffOptions& options = {},
    RetryPredicate&& should_retry = [](const std::exception&) { return true; }) {
    if (options.max_attempts == 0) {
        throw std::invalid_argument("max_attempts must be greater than zero");
    }

    std::random_device device;
    std::mt19937_64 random_engine(device());

    std::exception_ptr last_exception;

    for (std::size_t attempt = 0; attempt < options.max_attempts; ++attempt) {
        try {
            return std::invoke(std::forward<Operation>(operation));
        } catch (const std::exception& error) {
            last_exception = std::current_exception();

            if (attempt + 1 >= options.max_attempts ||
                !std::invoke(std::forward<RetryPredicate>(should_retry), error)) {
                std::rethrow_exception(last_exception);
            }

            std::this_thread::sleep_for(
                calculate_delay(attempt, options, random_engine));
        }
    }

    std::rethrow_exception(last_exception);
}

/// Executes an operation and retries failures using exponential backoff with jitter.
template <typename Operation>
decltype(auto) retry(
    Operation&& operation,
    const BackoffOptions& options = {}) {
    return retry(
        std::forward<Operation>(operation),
        options,
        [](const std::exception&) { return true; });
}

}  // namespace retry大小规律

