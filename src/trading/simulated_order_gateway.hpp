#pragma once

#include "core/types.hpp"
#include "trading/order_intent.hpp"

#include <cstddef>

class SimulatedOrderGateway {
public:
    explicit SimulatedOrderGateway(OrderId first_order_id = 1) : next_order_id_(first_order_id) {}

    bool submit(const OrderIntent &intent) {
        if (intent.qty == 0) {
            return false;
        }

        [[maybe_unused]] OrderId order_id = generate_order_id();
        ++submitted_count_;
        return true;
    }

    OrderId next_order_id() const {
        return next_order_id_;
    }

    std::size_t submitted_count() const {
        return submitted_count_;
    }

private:
    OrderId generate_order_id() {
        return next_order_id_++;
    }

    OrderId next_order_id_{1};
    std::size_t submitted_count_{0};
};
