#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

class CircuitBreaker {
public:
    using Clock = std::chrono::steady_clock;

    CircuitBreaker(
        std::size_t failure_threshold,
        std::chrono::milliseconds open_duration)
        : failure_threshold_(failure_threshold),
          open_duration_(open_duration) {
        if (failure_threshold_ == 0) {
            throw std::invalid_argument("failure threshold must be greater than zero");
        }
    }

    template <typename Function>
    decltype(auto) execute(Function&& function) {
        acquire_permission();

        try {
            if constexpr (std::is_void_v<std::invoke_result_t<Function>>) {
                std::invoke(std::forward<Function>(function));
                record_success();
            } else {
                auto result = std::invoke(std::forward<Function>(function));
                record_success();
                return result;
            }
        } catch (...) {
            record_failure();
            throw;
        }
    }

private:
    enum class State {
        Closed,
        Open,
        HalfOpen
    };

    void acquire_permission() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ == State::Closed) {
            return;
        }

        if (state_ == State::Open) {
            const auto elapsed = Clock::now() - opened_at_;

            if (elapsed >= open_duration_) {
                state_ = State::HalfOpen;
                probe_in_flight_ = true;
                return;
            }

            throw std::runtime_error("circuit breaker is open");
        }

        if (probe_in_flight_) {
            throw std::runtime_error("circuit breaker is half-open");
        }

        probe_in_flight_ = true;
    }

    void record_success() {
        std::lock_guard<std::mutex> lock(mutex_);

        failure_count_ = 0;
        probe_in_flight_ = false;
        state_ = State::Closed;
    }

    void record_failure() {
        std::lock_guard<std::mutex> lock(mutex_);

        probe_in_flight_ = false;

        if (state_ == State::HalfOpen ||
            ++failure_count_ >= failure_threshold_) {
            state_ = State::Open;
            opened_at_ = Clock::now();
        }
    }

    const std::size_t failure_threshold_;
    const std::chrono::milliseconds open_duration_;

    std::mutex mutex_;
    State state_ = State::Closed;
    std::size_t failure_count_ = 0;
    bool probe_in_flight_ = false;
    Clock::time_point opened_at_{};
};
