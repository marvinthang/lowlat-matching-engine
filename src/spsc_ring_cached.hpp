#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <cassert>
#include <bit>
#include <algorithm>

template <class T, std::size_t Capacity>
class SpscRingCached {
public:
    static_assert(Capacity > 1, "Capacity must be greater than 1");
    static_assert(std::has_single_bit(Capacity), "Capacity must be a power of 2");

    explicit SpscRingCached() : data_(std::make_unique<T[]>(Capacity)) {}

    bool push(const T &x) {
        std::size_t head = producer_.head.load(std::memory_order_relaxed);

        if (head - producer_.cached_tail >= Capacity) {
            producer_.cached_tail = consumer_.tail.load(std::memory_order_acquire);

            if (head - producer_.cached_tail >= Capacity) {
                return false;
            }
        }

        data_[head & (Capacity - 1)] = x;
        producer_.head.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T &x) {
        std::size_t tail = consumer_.tail.load(std::memory_order_relaxed);

        if (consumer_.cached_head == tail) {
            consumer_.cached_head = producer_.head.load(std::memory_order_acquire);

            if (consumer_.cached_head == tail) {
                return false;
            }
        }

        x = data_[tail & (Capacity - 1)];
        consumer_.tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    std::size_t push_many(const T *in, std::size_t max_count) {
        std::size_t head = producer_.head.load(std::memory_order_relaxed);

        std::size_t used = head - producer_.cached_tail;

        if (used >= Capacity) {
            producer_.cached_tail = consumer_.tail.load(std::memory_order_acquire);
            used = head - producer_.cached_tail;

            if (used >= Capacity) {
                return 0;
            }
        }

        std::size_t count = std::min(Capacity - used, max_count);

        if (count == 0) {
            return 0;
        }

        for (std::size_t i = 0; i < count; ++i) {
            data_[(head + i) & mask()] = in[i];
        }

        producer_.head.store(head + count, std::memory_order_release);
        return count;
    }

    std::size_t pop_many(T *out, std::size_t max_count) {
        std::size_t tail = consumer_.tail.load(std::memory_order_relaxed);

        if (consumer_.cached_head == tail) {
            consumer_.cached_head = producer_.head.load(std::memory_order_acquire);

            if (consumer_.cached_head == tail) {
                return 0;
            }
        }

        std::size_t count = std::min(consumer_.cached_head - tail, max_count);
        if (count == 0) {
            return 0;
        }

        for (std::size_t i = 0; i < count; ++i) {
            out[i] = data_[(tail + i) & mask()];
        }

        consumer_.tail.store(tail + count, std::memory_order_release);
        return count;
    }

private:
    constexpr static std::size_t mask() {
        return Capacity - 1;
    }

    std::unique_ptr<T[]> data_;

    struct alignas(64) ProducerState {
        std::atomic<std::size_t> head{0};
        std::size_t cached_tail{0};
    };

    struct alignas(64) ConsumerState {
        std::atomic<std::size_t> tail{0};
        std::size_t cached_head{0};
    };

    ProducerState producer_;
    ConsumerState consumer_;
};
