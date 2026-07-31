#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class MinHeapPriorityQueue {
public:
    bool empty() const noexcept {
        return heap_.empty();
    }

    std::size_t size() const noexcept {
        return heap_.size();
    }

    const T& top() const {
        if (heap_.empty()) {
            throw std::out_of_range("priority queue is empty");
        }
        return heap_.front();
    }

    void push(const T& value) {
        ensureCapacityForPush();
        heap_.push_back(value);
        siftUp(heap_.size() - 1);
    }

    void push(T&& value) {
        ensureCapacityForPush();
        heap_.push_back(std::move(value));
        siftUp(heap_.size() - 1);
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        ensureCapacityForPush();
        heap_.emplace_back(std::forward<Args>(args)...);
        siftUp(heap_.size() - 1);
    }

    void pop() {
        if (heap_.empty()) {
            throw std::out_of_range("priority queue is empty");
        }

        if (heap_.size() == 1) {
            heap_.pop_back();
            return;
        }

        heap_.front() = std::move(heap_.back());
        heap_.pop_back();
        siftDown(0);
    }

    void clear() noexcept {
        heap_.clear();
    }

private:
    std::vector<T> heap_;
    Compare compare_{};

    void ensureCapacityForPush() const {
        if (heap_.size() == std::numeric_limits<std::size_t>::max()) {
            throw std::overflow_error("priority queue size overflow");
        }
    }

    void siftUp(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;

            if (!compare_(heap_[index], heap_[parent])) {
                break;
            }

            std::swap(heap_[index], heap_[parent]);
            index = parent;
        }
    }

    void siftDown(std::size_t index) {
        const std::size_t count = heap_.size();

        while (count >= 2 && index <= (count - 2) / 2) {
            const std::size_t left = index * 2 + 1;
            std::size_t smallest = left;

            if (left < count - 1 && compare_(heap_[left + 1], heap_[left])) {
                smallest = left + 1;
            }

            if (!compare_(heap_[smallest], heap_[index])) {
                break;
            }

            std::swap(heap_[index], heap_[smallest]);
            index = smallest;
        }
    }
};
