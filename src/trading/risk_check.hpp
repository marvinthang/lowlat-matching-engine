#pragma once

#include "core/types.hpp"
#include "trading/order_intent.hpp"

#include <cstddef>
#include <cstdlib>

class RiskCheck {
public:
    RiskCheck(Quantity max_order_qty, Quantity max_abs_position, std::size_t max_orders)
        : max_order_qty_(max_order_qty),
          max_abs_position_(max_abs_position),
          max_orders_(max_orders) {}

    bool allow(const OrderIntent &intent) const {
        if (intent.qty > max_order_qty_) {
            return false;
        }

        if (orders_sent_ >= max_orders_) {
            return false;
        }

        Position new_position = position_ + (intent.side == Side::Buy ? intent.qty : -intent.qty);

        if (std::abs(new_position) > max_abs_position_) {
            return false;
        }

        return true;
    }

    void record_order(const OrderIntent &intent) {
        position_ += (intent.side == Side::Buy ? intent.qty : -intent.qty);
        ++orders_sent_;
    }

    Position position() const {
        return position_;
    }

    std::size_t orders_sent() const {
        return orders_sent_;
    }

private:
    Quantity max_order_qty_{0};
    Position max_abs_position_{0};
    std::size_t max_orders_{0};

    Position position_{0};
    std::size_t orders_sent_{0};
};
