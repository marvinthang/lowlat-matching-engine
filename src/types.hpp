#pragma once

#include <cstdint>
#include <limits>

using OrderId = std::uint64_t;
using Price = std::uint64_t;
using Quantity = std::uint64_t;
using OrderIndex = std::uint32_t;

enum class Side : std::uint8_t {
    Buy,
    Sell
};

constexpr OrderIndex null_order_index = std::numeric_limits<OrderIndex>::max();
