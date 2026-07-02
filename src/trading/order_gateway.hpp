#pragma once

#include "core/types.hpp"
#include "trading/order_intent.hpp"

#include <cstddef>

class OrderGateway {
public:
    explicit OrderGateway(OrderId first_order_id = 1) : next_order_id_(first_order_id) {}

    template <class Clob, class ExecutionSink>
    bool submit(const OrderIntent &intent, Clob &clob, ExecutionSink &sink) {
        if (intent.qty == 0) {
            return false;
        }

        OrderId order_id = generate_order_id();

        if (clob.submit_limit_order(order_id, intent.side, intent.price, intent.qty, sink)) {
            ++submitted_count_;
            return true;
        }

        return false;
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
