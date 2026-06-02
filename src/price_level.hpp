#pragma once

#include "types.hpp"

struct PriceLevel {
    OrderIndex head_idx{null_order_index};
    OrderIndex tail_idx{null_order_index};
    Quantity total_qty{0};
    std::size_t order_count{0};

    bool empty() const {
        return order_count == 0;
    }
};
