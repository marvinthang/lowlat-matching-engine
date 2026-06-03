#pragma once

#include "execution.hpp"

#include <cstddef>
#include <cassert>
#include <memory>
#include <algorithm>

class ExecutionBuffer {
   public:
    void reserve(std::size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        auto new_data = std::make_unique<Execution[]>(new_capacity);

        if (data_) {
            std::copy_n(data_.get(), size_, new_data.get());
        }

        data_ = std::move(new_data);
        capacity_ = new_capacity;
    }

    void emplace_back(OrderId incoming_order_id, OrderId resting_order_id, Price price,
                      Quantity qty) {
        assert(size_ < capacity_);
        data_[size_++] = Execution{incoming_order_id, resting_order_id, price, qty};
    }


    std::size_t size() const {
        return size_;
    }

    void clear() {
        size_ = 0;
    }

   private:
    std::unique_ptr<Execution[]> data_;
    std::size_t capacity_{0};
    std::size_t size_{0};
};

class NullExecutionSink {
   public:
    void reserve(std::size_t) {}

    void emplace_back(OrderId incoming_order_id, OrderId resting_order_id, Price price,
                      Quantity qty) {
        (void)incoming_order_id;
        (void)resting_order_id;
        (void)price;
        (void)qty;
    }

    std::size_t size() const {
        return 0;
    }

    void clear() {}
};
