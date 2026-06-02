#include "types.hpp"

#include <vector>

struct ClobOrder {
    OrderId order_id;
    Side side;
    Price price;
    Quantity qty;
};

inline std::vector<ClobOrder> make_orders(std::size_t n) {
    std::vector<ClobOrder> orders;
    orders.reserve(n);

    constexpr Price base_bid = 10000;
    constexpr Price base_ask = 10001;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Side side = (i & 1) ? Side::Buy : Side::Sell;

        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = (side == Side::Buy) ? base_bid - offset : base_ask + offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        orders.emplace_back(order_id, side, price, qty);
    }

    return orders;
}

inline std::vector<ClobOrder> make_resting_asks(std::size_t n) {
    std::vector<ClobOrder> orders;
    orders.reserve(n);

    constexpr Price base_ask = 10001;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = base_ask + offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        orders.emplace_back(order_id, Side::Sell, price, qty);
    }

    return orders;
}

inline std::vector<ClobOrder> make_non_crossing_buys(std::size_t n) {
    std::vector<ClobOrder> orders;
    orders.reserve(n);

    constexpr Price base_bid = 10000;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = base_bid - offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        orders.emplace_back(order_id, Side::Buy, price, qty);
    }

    return orders;
}
