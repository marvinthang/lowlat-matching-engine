#pragma once

#include "order.hpp"
#include <memory>
#include <optional>
#include <cassert>

template <std::size_t MaxOrders>
class OrderPool {
   public:
    OrderPool()
        : orders_(std::make_unique<Order[]>(MaxOrders)),
          free_stack_(std::make_unique<OrderIndex[]>(MaxOrders)) {
        for (OrderIndex i = 0; i < MaxOrders; ++i) {
            free_stack_[i] = i;
        }
    }

    std::optional<OrderIndex> allocate() {
        if (top_ == 0) {
            return std::nullopt;
        }

        OrderIndex idx = free_stack_[--top_];
        return idx;
    }

    void release(OrderIndex idx) {
        assert(idx < MaxOrders);
        assert(top_ < MaxOrders);

        free_stack_[top_++] = idx;
    }

    Order &get(OrderIndex idx) {
        assert(idx < MaxOrders);
        return orders_[idx];
    }

    const Order &get(OrderIndex idx) const {
        assert(idx < MaxOrders);
        return orders_[idx];
    }

    constexpr std::size_t capacity() const {
        return MaxOrders;
    }

    std::size_t available() const {
        return top_;
    }

    std::size_t used() const {
        return MaxOrders - top_;
    }

    bool full() const {
        return top_ == 0;
    }

    bool empty() const {
        return top_ == MaxOrders;
    }

   private:
    std::unique_ptr<Order[]> orders_;
    std::unique_ptr<OrderIndex[]> free_stack_;
    std::size_t top_{MaxOrders};
};
