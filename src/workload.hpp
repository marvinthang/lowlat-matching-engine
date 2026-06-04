#include "order_command.hpp"

#include <vector>
#include <cstddef>

inline std::vector<OrderCommand> make_add_only_commands(std::size_t n) {
    std::vector<OrderCommand> commands;
    commands.reserve(n);

    constexpr Price base_bid = 10000;
    constexpr Price base_ask = 10001;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Side side = (i & 1) ? Side::Buy : Side::Sell;

        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = (side == Side::Buy) ? base_bid - offset : base_ask + offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        commands.push_back(OrderCommand{.order_id = order_id,
                                        .price = price,
                                        .qty = qty,
                                        .type = CommandType::AddLimit,
                                        .side = side});
    }

    return commands;
}

inline std::vector<OrderCommand> make_resting_asks_commands(std::size_t n) {
    std::vector<OrderCommand> commands;
    commands.reserve(n);

    constexpr Price base_ask = 10001;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = base_ask + offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        commands.push_back(OrderCommand{.order_id = order_id,
                                        .price = price,
                                        .qty = qty,
                                        .type = CommandType::AddLimit,
                                        .side = Side::Sell});
    }

    return commands;
}

inline std::vector<OrderCommand> make_non_crossing_buys_commands(std::size_t n) {
    std::vector<OrderCommand> commands;
    commands.reserve(n);

    constexpr Price base_bid = 10000;
    constexpr Price levels = 1024;

    for (std::size_t i = 0; i < n; ++i) {
        Price offset = static_cast<Price>((i * 37) % levels);
        Price price = base_bid - offset;

        Quantity qty = static_cast<Quantity>(i % 100 + 1);
        OrderId order_id = static_cast<OrderId>(i + 1);

        commands.push_back(OrderCommand{.order_id = order_id,
                                        .price = price,
                                        .qty = qty,
                                        .type = CommandType::AddLimit,
                                        .side = Side::Buy});
    }

    return commands;
}

inline std::vector<OrderCommand> make_full_match_commands(std::size_t n) {
    std::vector<OrderCommand> commands;
    commands.reserve(2 * n);

    constexpr Price price = 10000;

    for (std::size_t i = 0; i < n; ++i) {
        OrderId order_id = static_cast<OrderId>(i + 1);
        Quantity qty = static_cast<Quantity>(i % 100 + 1);

        commands.push_back(OrderCommand{.order_id = order_id,
                                        .price = price,
                                        .qty = qty,
                                        .type = CommandType::AddLimit,
                                        .side = Side::Sell});
    }

    for (std::size_t i = 0; i < n; ++i) {
        OrderId order_id = static_cast<OrderId>(n + i + 1);
        Quantity qty = static_cast<Quantity>(i % 100 + 1);

        commands.push_back(OrderCommand{.order_id = order_id,
                                        .price = price,
                                        .qty = qty,
                                        .type = CommandType::AddLimit,
                                        .side = Side::Buy});
    }

    return commands;
}
