#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <cassert>
#include <bit>
#include <algorithm>

template <class T, std::size_t Capacity>
class SpscRing {
public:
    static_assert(Capacity > 1, "Capacity must be greater than 1");
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of 2");

    explicit SpscRing() : data_(std::make_unique<T[]>(Capacity)) {}

    bool push(const T &x) {
        std::size_t head = head_.load(std::memory_order_relaxed);
        std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head - tail >= Capacity) {
            return false;
        }

        data_[head & (Capacity - 1)] = x;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T &x) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t head = head_.load(std::memory_order_acquire);

        if (head == tail) {
            return false;
        }

        x = data_[tail & (Capacity - 1)];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    std::size_t pop_many(T *out, std::size_t max_count) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t head = head_.load(std::memory_order_acquire);

        std::size_t count = std::min(head - tail, max_count);
        if (count == 0) {
            return 0;
        }

        for (std::size_t i = 0; i < count; ++i) {
            out[i] = data_[(tail + i) & mask()];
        }

        tail_.store(tail + count, std::memory_order_release);
        return count;
    }

private:
    constexpr static std::size_t mask() {
        return Capacity - 1;
    }

    std::unique_ptr<T[]> data_;

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};
