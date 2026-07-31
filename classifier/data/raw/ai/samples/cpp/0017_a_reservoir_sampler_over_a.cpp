#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

template <typename T>
class ReservoirSampler {
public:
    explicit ReservoirSampler(std::size_t capacity)
        : capacity_(capacity), seen_(0), rng_(std::random_device{}()) {
        if (capacity_ == 0) {
            throw std::invalid_argument("Reservoir capacity must be greater than zero");
        }
        reservoir_.reserve(capacity_);
    }

    void add(const T& item) {
        ++seen_;

        if (reservoir_.size() < capacity_) {
            reservoir_.push_back(item);
            return;
        }

        std::uniform_int_distribution<std::size_t> distribution(0, seen_ - 1);
        const std::size_t index = distribution(rng_);

        if (index < capacity_) {
            reservoir_[index] = item;
        }
    }

    void add(T&& item) {
        ++seen_;

        if (reservoir_.size() < capacity_) {
            reservoir_.push_back(std::move(item));
            return;
        }

        std::uniform_int_distribution<std::size_t> distribution(0, seen_ - 1);
        const std::size_t index = distribution(rng_);

        if (index < capacity_) {
            reservoir_[index] = std::move(item);
        }
    }

    const std::vector<T>& sample() const noexcept {
        return reservoir_;
    }

    std::size_t size() const noexcept {
        return reservoir_.size();
    }

    std::size_t seen() const noexcept {
        return seen_;
    }

private:
    std::size_t capacity_;
    std::size_t seen_;
    std::vector<T> reservoir_;
    std::mt19937_64 rng_;
};
