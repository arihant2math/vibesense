#include <cstdint>
#include <limits>

class PseudoRandom {
private:
    std::uint64_t state_;

    static std::uint64_t splitMix64(std::uint64_t& value) {
        value += 0x9E3779B97F4A7C15ULL;
        std::uint64_t result = value;
        result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ULL;
        result = (result ^ (result >> 27)) * 0x94D049BB133111EBULL;
        return result ^ (result >> 31);
    }

public:
    /**
     * Constructs a pseudo-random number generator with the specified seed.
     */
    explicit PseudoRandom(std::uint64_t seed = 0) {
        this->seed(seed);
    }

    /**
     * Reseeds the generator and resets its internal state.
     */
    void seed(std::uint64_t value) {
        state_ = splitMix64(value);
        if (state_ == 0) {
            state_ = 0xA5A5A5A5A5A5A5A5ULL;
        }
    }

    /**
     * Generates the next uniformly distributed 64-bit unsigned integer.
     */
    std::uint64_t nextUInt64() {
        std::uint64_t value = state_;
        value ^= value >> 12;
        value ^= value << 25;
        value ^= value >> 27;
        state_ = value;
        return value * 0x2545F4914F6CDD1DULL;
    }

    /**
     * Generates a uniformly distributed integer in the range [0, bound).
     *
     * Returns zero when bound is zero.
     */
    std::uint64_t nextBounded(std::uint64_t bound) {
        if (bound == 0) {
            return 0;
        }

        const std::uint64_t threshold = -bound % bound;
        for (;;) {
            const std::uint64_t value = nextUInt64();
            if (value >= threshold) {
                return value % bound;
            }
        }
    }

    /**
     * Generates a uniformly distributed floating-point value in [0.0, 1.0).
     */
    double nextDouble() {
        return static_cast<double>(nextUInt64() >> 11) *
               (1.0 / 9007199254740992.0);
    }
};
