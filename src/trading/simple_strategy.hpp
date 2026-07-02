#pragma once

#include "core/types.hpp"
#include "trading/local_order_book.hpp"
#include "trading/order_intent.hpp"

class SimpleStrategy {
public:
    using Level = LocalOrderBook::Level;
    SimpleStrategy(Price max_spread, Quantity order_qty)
        : max_spread_(max_spread), order_qty_(order_qty) {}

    std::optional<OrderIntent> on_book_update(const LocalOrderBook &book) const {
        if (!book.has_ask() || !book.has_bid()) {
            return std::nullopt;
        }

        if (book.spread() > max_spread_) {
            return std::nullopt;
        }

        std::optional<Level> best_bid = book.best_bid();
        if (best_bid.has_value()) {
            return OrderIntent{best_bid->first + 1, order_qty_, Side::Buy};
        }

        std::optional<Level> best_ask = book.best_ask();
        assert(best_ask.has_value());

        return OrderIntent{best_ask->first - 1, order_qty_, Side::Sell};
    }

private:
    Price max_spread_;
    Quantity order_qty_;
};
