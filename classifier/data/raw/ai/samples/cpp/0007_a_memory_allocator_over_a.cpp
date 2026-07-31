#include <cstddef>
#include <cstdint>

class FixedArenaAllocator {
    struct Block {
        std::size_t size;
        Block* prev_free;
        Block* next_free;
        bool free;
    };

    static constexpr std::size_t Alignment = alignof(std::max_align_t);

    std::byte* arena_;
    std::size_t capacity_;
    Block* free_list_;

    static std::uintptr_t align_up(std::uintptr_t value) {
        return (value + Alignment - 1) & ~(Alignment - 1);
    }

    static std::size_t align_size(std::size_t size) {
        return (size + Alignment - 1) & ~(Alignment - 1);
    }

    void remove_free(Block* block) {
        if (block->prev_free)
            block->prev_free->next_free = block->next_free;
        else
            free_list_ = block->next_free;

        if (block->next_free)
            block->next_free->prev_free = block->prev_free;

        block->prev_free = nullptr;
        block->next_free = nullptr;
    }

    void insert_free(Block* block) {
        block->free = true;
        block->prev_free = nullptr;
        block->next_free = free_list_;

        if (free_list_)
            free_list_->prev_free = block;

        free_list_ = block;
    }

    Block* next_block(Block* block) const {
        auto* next = reinterpret_cast<std::byte*>(block) +
                     sizeof(Block) + block->size;

        auto* end = arena_ + capacity_;
        return next < end ? reinterpret_cast<Block*>(next) : nullptr;
    }

    Block* previous_block(Block* block) const {
        Block* current = reinterpret_cast<Block*>(arena_);
        Block* previous = nullptr;

        while (current && current != block) {
            previous = current;
            current = next_block(current);
        }

        return previous;
    }

public:
    FixedArenaAllocator(void* arena, std::size_t capacity)
        : arena_(nullptr), capacity_(0), free_list_(nullptr) {
        if (!arena || capacity < sizeof(Block))
            return;

        auto begin = reinterpret_cast<std::uintptr_t>(arena);
        auto aligned = align_up(begin);
        auto adjustment = static_cast<std::size_t>(aligned - begin);

        if (adjustment >= capacity ||
            capacity - adjustment < sizeof(Block))
            return;

        arena_ = reinterpret_cast<std::byte*>(aligned);
        capacity_ = capacity - adjustment;

        auto* initial = reinterpret_cast<Block*>(arena_);
        initial->size = capacity_ - sizeof(Block);
        initial->prev_free = nullptr;
        initial->next_free = nullptr;
        initial->free = true;
        free_list_ = initial;
    }

    void* allocate(std::size_t size) {
        if (!size || !free_list_)
            return nullptr;

        size = align_size(size);

        for (Block* block = free_list_; block; block = block->next_free) {
            if (block->size < size)
                continue;

            remove_free(block);
            block->free = false;

            if (block->size >= size + sizeof(Block) + Alignment) {
                auto remaining_size = block->size - size - sizeof(Block);
                auto* remaining = reinterpret_cast<Block*>(
                    reinterpret_cast<std::byte*>(block) +
                    sizeof(Block) + size);

                remaining->size = remaining_size;
                remaining->prev_free = nullptr;
                remaining->next_free = nullptr;
                remaining->free = true;

                block->size = size;
                insert_free(remaining);
            }

            return reinterpret_cast<std::byte*>(block) + sizeof(Block);
        }

        return nullptr;
    }

    void deallocate(void* pointer) {
        if (!pointer)
            return;

        auto* block = reinterpret_cast<Block*>(
            static_cast<std::byte*>(pointer) - sizeof(Block));

        if (block->free)
            return;

        block->free = true;
        block->prev_free = nullptr;
        block->next_free = nullptr;

        Block* next = next_block(block);
        if (next && next->free) {
            remove_free(next);
            block->size += sizeof(Block) + next->size;
        }

        Block* previous = previous_block(block);
        if (previous && previous->free) {
            remove_free(previous);
            previous->size += sizeof(Block) + block->size;
            block = previous;
        }

        insert_free(block);
    }

    void reset() {
        if (!arena_ || capacity_ < sizeof(Block))
            return;

        auto* initial = reinterpret_cast<Block*>(arena_);
        initial->size = capacity_ - sizeof(Block);
        initial->prev_free = nullptr;
        initial->next_free = nullptr;
        initial->free = true;
        free_list_ = initial;
    }

    std::size_t available() const {
        std::size_t total = 0;

        for (Block* block = free_list_; block; block = block->next_free)
            total += block->size;

        return total;
    }
};
