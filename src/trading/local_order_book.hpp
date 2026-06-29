#pragma once

#include "core/types.hpp"
#include "market_data/market_event.hpp"

#include <cstddef>
#include <cassert>
#include <memory>
#include <optional>
#include <utility>

class LocalOrderBook {
public:
    using Level = std::pair<Price, Quantity>;

    LocalOrderBook(Price min_price, Price max_price)
        : min_price_(min_price),
          max_price_(max_price),
          bid_levels_(std::make_unique<Quantity[]>(range_size())),
          ask_levels_(std::make_unique<Quantity[]>(range_size())) {}

    bool on_event(const MarketEvent &event) {
        if (!valid_price(event.price)) {
            return false;
        }

        auto &lvl = level(event.side, event.price);

        lvl = event.qty;

        if (lvl == 0) {
            refresh_best_after_empty_level(event.side, event.price);
        } else {
            update_best_after_add(event.side, event.price);
        }

        return true;
    }

    bool has_bid() const {
        return best_bid_.has_value();
    }

    bool has_ask() const {
        return best_ask_.has_value();
    }

    std::optional<Level> best_bid() const {
        if (!best_bid_.has_value()) {
            return std::nullopt;
        }

        Price price = *best_bid_;
        return Level{price, bid_levels_[index(price)]};
    }

    std::optional<Level> best_ask() const {
        if (!best_ask_.has_value()) {
            return std::nullopt;
        }

        Price price = *best_ask_;
        return Level{price, ask_levels_[index(price)]};
    }

    std::optional<Price> spread() const {
        if (!best_bid_.has_value() || !best_ask_.has_value()) {
            return std::nullopt;
        }

        assert(*best_bid_ <= *best_ask_);

        return *best_ask_ - *best_bid_;
    }

    Quantity quantity_at(Side side, Price price) const {
        if (!valid_price(price)) {
            return 0;
        }

        return side == Side::Buy ? bid_levels_[index(price)] : ask_levels_[index(price)];
    }

private:
    void update_best_after_add(Side side, Price price) {
        if (side == Side::Buy) {
            if (!best_bid_.has_value() || price > *best_bid_) {
                best_bid_ = price;
            }
        } else {
            if (!best_ask_.has_value() || price < *best_ask_) {
                best_ask_ = price;
            }
        }
    }

    void refresh_best_after_empty_level(Side side, Price price) {
        if (side == Side::Buy) {
            if (!best_bid_.has_value() || price != *best_bid_)
                return;

            while (price > min_price_) {
                --price;

                if (bid_levels_[index(price)] > 0) {
                    best_bid_ = price;
                    return;
                }
            }

            best_bid_ = std::nullopt;
        } else {
            if (!best_ask_.has_value() || price != *best_ask_)
                return;

            while (price < max_price_) {
                ++price;

                if (ask_levels_[index(price)] > 0) {
                    best_ask_ = price;
                    return;
                }
            }

            best_ask_ = std::nullopt;
        }
    }

    bool valid_price(Price price) const {
        return min_price_ <= price && price <= max_price_;
    }

    std::size_t index(Price price) const {
        assert(valid_price(price));
        return static_cast<std::size_t>(price - min_price_);
    }

    Quantity &level(Side side, Price price) {
        return (side == Side::Buy) ? bid_levels_[index(price)] : ask_levels_[index(price)];
    }

    std::size_t range_size() const {
        assert(min_price_ <= max_price_);
        return static_cast<std::size_t>(max_price_ - min_price_ + 1);
    }

    Price min_price_;
    Price max_price_;

    std::unique_ptr<Quantity[]> bid_levels_;
    std::unique_ptr<Quantity[]> ask_levels_;

    std::optional<Price> best_bid_;
    std::optional<Price> best_ask_;
};
