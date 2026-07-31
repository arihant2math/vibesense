#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>

template <std::size_t Capacity>
class FixedArenaAllocator {
    static_assert(Capacity > 0, "Arena capacity must be greater than zero");

    struct alignas(std::max_align_t) Block {
        std::size_t size;
        bool free;
        Block* next;
        Block* previous;
    };

    static constexpr std::size_t Alignment = alignof(std::max_align_t);
    static constexpr std::size_t HeaderSize =
        (sizeof(Block) + Alignment - 1) & ~(Alignment - 1);

    alignas(std::max_align_t) std::array<std::byte, Capacity> arena_{};
    Block* head_ = nullptr;

    static constexpr std::size_t alignUp(std::size_t value,
                                         std::size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    void split(Block* block, std::size_t requiredSize) noexcept {
        const std::size_t remainder = block->size - requiredSize;

        if (remainder < HeaderSize + Alignment) {
            return;
        }

        auto* next = reinterpret_cast<Block*>(
            reinterpret_cast<std::byte*>(block) + requiredSize);

        next->size = remainder;
        next->free = true;
        next->previous = block;
        next->next = block->next;

        if (next->next) {
            next->next->previous = next;
        }

        block->size = requiredSize;
        block->next = next;
    }

    void mergeWithNext(Block* block) noexcept {
        Block* next = block->next;

        if (!next || !next->free) {
            return;
        }

        block->size += next->size;
        block->next = next->next;

        if (block->next) {
            block->next->previous = block;
        }
    }

public:
    FixedArenaAllocator() noexcept {
        static_assert(Capacity >= HeaderSize,
                      "Arena capacity is too small for allocator metadata");

        head_ = reinterpret_cast<Block*>(arena_.data());
        head_->size = Capacity;
        head_->free = true;
        head_->next = nullptr;
        head_->previous = nullptr;
    }

    FixedArenaAllocator(const FixedArenaAllocator&) = delete;
    FixedArenaAllocator& operator=(const FixedArenaAllocator&) = delete;

    void* allocate(std::size_t bytes,
                   std::size_t alignment = Alignment) {
        if (bytes == 0) {
            bytes = 1;
        }

        if (alignment == 0 ||
            (alignment & (alignment - 1)) != 0 ||
            alignment > Alignment) {
            throw std::bad_alloc();
        }

        const std::size_t requiredSize = alignUp(HeaderSize + bytes, Alignment);

        for (Block* block = head_; block; block = block->next) {
            if (block->free && block->size >= requiredSize) {
                split(block, requiredSize);
                block->free = false;

                return reinterpret_cast<std::byte*>(block) + HeaderSize;
            }
        }

        throw std::bad_alloc();
    }

    void deallocate(void* pointer) noexcept {
        if (!pointer) {
            return;
        }

        auto* block = reinterpret_cast<Block*>(
            reinterpret_cast<std::byte*>(pointer) - HeaderSize);

        block->free = true;

        if (block->next && block->next->free) {
            mergeWithNext(block);
        }

        if (block->previous && block->previous->free) {
            mergeWithNext(block->previous);
        }
    }

    void reset() noexcept {
        head_ = reinterpret_cast<Block*>(arena_.data());
        head_->size = Capacity;
        head_->free = true;
        head_->next = nullptr;
        head_->previous = nullptr;
    }

    std::size_t capacity() const noexcept {
        return Capacity;
    }

    std::size_t available() const noexcept {
        std::size_t result = 0;

        for (Block* block = head_; block; block = block->next) {
            if (block->free) {
                result += block->size - HeaderSize;
            }
        }

        return result;
    }
};
