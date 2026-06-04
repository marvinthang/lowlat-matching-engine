#pragma once

#include "types.hpp"
#include <cstdint>

enum class CommandType : std::uint8_t {
    AddLimit,
    Cancel,
};

struct OrderCommand {
    OrderId order_id{0};
    Price price{0};
    Quantity qty{0};
    CommandType type{CommandType::AddLimit};
    Side side{Side::Buy};
};

static_assert(sizeof(OrderCommand) == 32, "OrderCommand should stay compact for the SPSC ring");
