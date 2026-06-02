#pragma once

#include "types.hpp"

struct Order {
    OrderId order_id{0};
    Price price{0};
    Quantity qty{0};
    OrderIndex next_idx{null_order_index};
    OrderIndex prev_idx{null_order_index};
    Side side{Side::Buy};
};
