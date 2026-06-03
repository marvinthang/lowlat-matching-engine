#pragma once

#include "types.hpp"
#include "order_pool.hpp"
#include "price_level.hpp"
#include "order_index.hpp"

#include <vector>
#include <optional>
#include <cassert>
#include <utility>

template <std::size_t MaxOrders, class OrderIdHash = FastOrderIdHash>
class FixedClob {
   public:
    using Level = std::pair<Price, Quantity>;

    FixedClob(Price min_price, Price max_price)
        : min_price_(min_price),
          max_price_(max_price),
          bid_levels_(range_size()),
          ask_levels_(range_size()) {}

    template <class ExecutionSink>
    bool submit_limit_order(OrderId order_id, Side side, Price price, Quantity qty,
                            ExecutionSink &executions) {
        if (qty == 0 || !valid_price(price)) {
            return false;
        }

        if (order_lookup_.contains(order_id)) {
            return false;
        }

        match_order(order_id, side, price, qty, executions);

        if (qty == 0) {
            return true;
        }

        return add_order(order_id, side, price, qty);
    }

    bool add_order(OrderId order_id, Side side, Price price, Quantity qty) {
        if (qty == 0 || !valid_price(price)) {
            return false;
        }

        auto maybe_idx = pool_.allocate();
        if (!maybe_idx.has_value()) {
            return false;
        }

        OrderIndex idx = *maybe_idx;

        if (!order_lookup_.insert(order_id, idx)) {
            pool_.release(idx);
            return false;
        }

        Order &order = pool_.get(idx);

        order.order_id = order_id;
        order.price = price;
        order.qty = qty;
        order.side = side;

        PriceLevel &lvl = level(side, price);
        append_to_level(lvl, idx);

        update_best_after_add(side, price);

        return true;
    }

    bool cancel_order(OrderId order_id) {
        auto maybe_idx = order_lookup_.find(order_id);
        if (!maybe_idx.has_value()) {
            return false;
        }

        remove_order_by_index(*maybe_idx);
        return true;
    }

    std::optional<Level> best_bid() const {
        if (!best_bid_.has_value()) {
            return std::nullopt;
        }

        Price price = *best_bid_;
        return Level{price, bid_levels_[index(price)].total_qty};
    }

    std::optional<Level> best_ask() const {
        if (!best_ask_.has_value()) {
            return std::nullopt;
        }

        Price price = *best_ask_;
        return Level{price, ask_levels_[index(price)].total_qty};
    }

    Quantity quantity_at(Side side, Price price) const {
        if (!valid_price(price)) {
            return 0;
        }

        return level(side, price).total_qty;
    }

    std::size_t order_count_at(Side side, Price price) const {
        if (!valid_price(price)) {
            return 0;
        }

        return level(side, price).order_count;
    }

    std::size_t active_orders() const {
        return pool_.used();
    }

    std::size_t available_slots() const {
        return pool_.available();
    }

    bool empty() const {
        return pool_.empty();
    }

    std::optional<OrderId> front_order_id(Side side, Price price) const {
        if (!valid_price(price)) {
            return std::nullopt;
        }

        const auto &lvl = level(side, price);
        if (lvl.head_idx == null_order_index) {
            return std::nullopt;
        }

        return pool_.get(lvl.head_idx).order_id;
    }

    std::optional<OrderId> back_order_id(Side side, Price price) const {
        if (!valid_price(price)) {
            return std::nullopt;
        }

        const auto &lvl = level(side, price);
        if (lvl.tail_idx == null_order_index) {
            return std::nullopt;
        }

        return pool_.get(lvl.tail_idx).order_id;
    }

   private:
    std::size_t range_size() const {
        assert(min_price_ <= max_price_);
        return static_cast<std::size_t>(max_price_ - min_price_ + 1);
    }

    bool valid_price(Price price) const {
        return min_price_ <= price && price <= max_price_;
    }

    std::size_t index(Price price) const {
        assert(valid_price(price));
        return static_cast<std::size_t>(price - min_price_);
    }

    PriceLevel &level(Side side, Price price) {
        return (side == Side::Buy) ? bid_levels_[index(price)] : ask_levels_[index(price)];
    }

    const PriceLevel &level(Side side, Price price) const {
        return (side == Side::Buy) ? bid_levels_[index(price)] : ask_levels_[index(price)];
    }

    PriceLevel &best_opposite_level(Side incoming_side) {
        if (incoming_side == Side::Buy) {
            assert(best_ask_.has_value());
            return level(Side::Sell, *best_ask_);
        }
        assert(best_bid_.has_value());
        return level(Side::Buy, *best_bid_);
    }

    bool crosses(Side incoming_side, Price incoming_price) const {
        if (incoming_side == Side::Buy) {
            return best_ask_.has_value() && *best_ask_ <= incoming_price;
        }
        return best_bid_.has_value() && *best_bid_ >= incoming_price;
    }

    template <class ExecutionSink>
    void match_order(OrderId incoming_order_id, Side incoming_side, Price incoming_price,
                     Quantity &incoming_qty, ExecutionSink &executions) {
        while (incoming_qty > 0 && crosses(incoming_side, incoming_price)) {
            auto &lvl = best_opposite_level(incoming_side);
            match_at_level(lvl, incoming_order_id, incoming_qty, executions);
        }
    }

    template <class ExecutionSink>
    void match_at_level(PriceLevel &lvl, OrderId incoming_order_id, Quantity &incoming_qty,
                        ExecutionSink &executions) {
        while (incoming_qty > 0 && lvl.order_count > 0) {
            OrderIndex resting_order_idx = lvl.head_idx;
            auto &resting_order = pool_.get(resting_order_idx);

            if (incoming_qty < resting_order.qty) {
                executions.emplace_back(incoming_order_id, resting_order.order_id,
                                        resting_order.price, incoming_qty);

                reduce_order_quantity(resting_order_idx, incoming_qty, lvl);
                incoming_qty = 0;

                break;
            }

            executions.emplace_back(incoming_order_id, resting_order.order_id, resting_order.price,
                                    resting_order.qty);

            incoming_qty -= resting_order.qty;

            remove_order_by_index(resting_order_idx);
        }
    }

    void reduce_order_quantity(OrderIndex idx, Quantity traded_qty, PriceLevel &lvl) {
        Order &order = pool_.get(idx);

        assert(order.qty > traded_qty);

        order.qty -= traded_qty;
        lvl.total_qty -= traded_qty;
    }

    void remove_order_by_index(OrderIndex idx) {
        Order &order = pool_.get(idx);
        Side side = order.side;
        Price price = order.price;

        PriceLevel &lvl = level(side, price);

        unlink_from_level(lvl, idx);

        order_lookup_.erase(order.order_id);
        pool_.release(idx);

        if (lvl.empty()) {
            refresh_best_after_empty_level(side, price);
        }
    }

    void append_to_level(PriceLevel &lvl, OrderIndex idx) {
        Order &order = pool_.get(idx);
        if (lvl.empty()) {
            lvl.head_idx = idx;
            lvl.tail_idx = idx;
            order.prev_idx = null_order_index;
            order.next_idx = null_order_index;
        } else {
            Order &tail_order = pool_.get(lvl.tail_idx);
            tail_order.next_idx = idx;

            order.prev_idx = lvl.tail_idx;
            order.next_idx = null_order_index;

            lvl.tail_idx = idx;
        }

        lvl.total_qty += order.qty;
        ++lvl.order_count;
    }

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

    void unlink_from_level(PriceLevel &lvl, OrderIndex idx) {
        Order &order = pool_.get(idx);

        OrderIndex prev_idx = order.prev_idx;
        OrderIndex next_idx = order.next_idx;

        if (prev_idx != null_order_index) {
            pool_.get(prev_idx).next_idx = next_idx;
        } else {
            lvl.head_idx = next_idx;
        }

        if (next_idx != null_order_index) {
            pool_.get(next_idx).prev_idx = prev_idx;
        } else {
            lvl.tail_idx = prev_idx;
        }

        order.prev_idx = null_order_index;
        order.next_idx = null_order_index;

        lvl.total_qty -= order.qty;
        --lvl.order_count;
    }

    void refresh_best_after_empty_level(Side side, Price price) {
        if (side == Side::Buy) {
            if (!best_bid_.has_value() || price != *best_bid_)
                return;

            while (price > min_price_) {
                --price;

                if (!bid_levels_[index(price)].empty()) {
                    best_bid_ = price;
                    return;
                }
            }

            best_bid_.reset();
        } else {
            if (!best_ask_.has_value() || price != *best_ask_)
                return;

            while (price < max_price_) {
                ++price;

                if (!ask_levels_[index(price)].empty()) {
                    best_ask_ = price;
                    return;
                }
            }

            best_ask_.reset();
        }
    }

    Price min_price_;
    Price max_price_;

    OrderPool<MaxOrders> pool_;
    std::vector<PriceLevel> bid_levels_;
    std::vector<PriceLevel> ask_levels_;

    FixedOrderIndex<2 * MaxOrders, OrderIdHash> order_lookup_;

    std::optional<Price> best_bid_;
    std::optional<Price> best_ask_;
};
