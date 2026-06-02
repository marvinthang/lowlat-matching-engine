#pragma once

#include "types.hpp"

#include <memory>
#include <bit>
#include <optional>
#include <cassert>
#include <algorithm>
#include <cstdint>

template<std::size_t Capacity>
class FixedOrderIndex {
public:
    FixedOrderIndex()
        : keys_(std::make_unique_for_overwrite<OrderId[]>(Capacity)),
          values_(std::make_unique_for_overwrite<OrderIndex[]>(Capacity)),
          states_(std::make_unique_for_overwrite<State[]>(Capacity)) {
            static_assert(Capacity > 1, "Capacity must be greater than 1");
            static_assert(std::has_single_bit(Capacity), "Capacity must be a power of 2");
            clear();
    }

    bool insert(OrderId order_id, OrderIndex idx) {
        assert(order_id != 0);

        std::size_t pos = hash(order_id) & mask();
        std::size_t first_deleted = Capacity;

        for (std::size_t step = 0; step < Capacity; ++step, pos = (pos + 1) & mask()) {
            if (states_[pos] == State::Empty) {
                if (first_deleted == Capacity) {
                    first_deleted = pos;
                }
                break;
            }
            if (states_[pos] == State::Deleted) {
                if (first_deleted == Capacity) {
                    first_deleted = pos;
                }
            } else if (keys_[pos] == order_id) {
                return false;
            }
        }

        if (first_deleted == Capacity) {
            return false;
        }

        keys_[first_deleted] = order_id;
        values_[first_deleted] = idx;
        states_[first_deleted] = State::Occupied;
        ++size_;
        return true;
    }

    std::optional<OrderIndex> find(OrderId order_id) const {
        assert(order_id != 0);

        std::size_t pos = hash(order_id) & mask();

        for (std::size_t step = 0; step < Capacity; ++step, pos = (pos + 1) & mask()) {
            if (states_[pos] == State::Empty) {
                return std::nullopt;
            }

            if (states_[pos] == State::Occupied && keys_[pos] == order_id) {
                return values_[pos];
            }
        }

        return std::nullopt;
    }

    bool erase(OrderId order_id) {
        assert(order_id != 0);

        std::size_t pos = hash(order_id) & mask();

        for (std::size_t step = 0; step < Capacity; ++step, pos = (pos + 1) & mask()) {
            if (states_[pos] == State::Empty) {
                return false;
            }

            if (states_[pos] == State::Occupied && keys_[pos] == order_id) {
                states_[pos] = State::Deleted;
                --size_;
                return true;
            }
        }

        return false;
    }

    bool contains(OrderId order_id) const {
        return find(order_id).has_value();
    }

    void clear() {
        std::fill_n(states_.get(), Capacity, State::Empty);
        size_ = 0;
    }

    constexpr std::size_t capacity() const {
        return Capacity;
    }

    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    bool full() const {
        return size_ == Capacity;
    }

private:
    enum class State : std::uint8_t {
        Empty,
        Occupied,
        Deleted,
    };

    static constexpr std::size_t mask() {
        return Capacity - 1;
    }

    static std::size_t hash(OrderId x) {
        x ^= x >> 32;
        return static_cast<std::size_t>(x);
    }

    std::unique_ptr<OrderId[]> keys_;
    std::unique_ptr<OrderIndex[]> values_;
    std::unique_ptr<State[]> states_;
    std::size_t size_{0};
};
