#include "core/types.hpp"

#include "infra/latency_stats.hpp"

#include "market_data/market_event.hpp"

#include "trading/local_order_book.hpp"
#include "trading/simple_strategy.hpp"
#include "trading/risk_check.hpp"
#include "trading/simulated_order_gateway.hpp"
#include "trading/order_intent.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr Price kMinPrice = 9000;
constexpr Price kMaxPrice = 11000;
constexpr Price kLowestBid = 9995;
constexpr Price kHighestBid = 10003;
constexpr Price kLowestAsk = 10004;
constexpr Price kHighestAsk = 10009;
constexpr Price kWideBestBid = 10001;

constexpr std::size_t kNumEvents = 1000000;
constexpr std::size_t kRegimeEventCount = 16;

constexpr Price kMaxSpread = 2;
constexpr Quantity kStrategyQty = 10;

constexpr Quantity kMaxOrderQty = 100;
constexpr Position kMaxPosition = 2'500'000;
constexpr Quantity kMaxMarketQty = 1000;
constexpr std::size_t kMaxOrders = kNumEvents;

std::vector<MarketEvent> make_events(std::size_t n) {
    std::mt19937_64 rng(0x5eed1234);
    std::uniform_int_distribution<Price> lower_bid_price_dist(kLowestBid, kWideBestBid);
    std::uniform_int_distribution<Price> higher_ask_price_dist(kLowestAsk + 1, kHighestAsk);
    std::uniform_int_distribution<Quantity> market_qty_dist(1, kMaxMarketQty);

    std::vector<MarketEvent> events;
    events.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        const Timestamp ts = static_cast<Timestamp>(i);
        const std::size_t phase = i % kRegimeEventCount;

        if (phase == 0) {
            events.emplace_back(Side::Buy, kHighestBid, 0, ts);
            continue;
        }

        if (phase == 1) {
            events.emplace_back(Side::Buy, kHighestBid - 1, 0, ts);
            continue;
        }

        if (phase < 8) {
            if (phase & 1) {
                events.emplace_back(Side::Sell, higher_ask_price_dist(rng), market_qty_dist(rng),
                                    ts);
            } else {
                events.emplace_back(Side::Buy, lower_bid_price_dist(rng), market_qty_dist(rng), ts);
            }
            continue;
        }

        if (phase == 8) {
            events.emplace_back(Side::Buy, kHighestBid - 1, market_qty_dist(rng), ts);
            continue;
        }

        if (phase == 9) {
            events.emplace_back(Side::Buy, kHighestBid, market_qty_dist(rng), ts);
            continue;
        }

        if (phase & 1) {
            events.emplace_back(Side::Sell, kLowestAsk, market_qty_dist(rng), ts);
        } else {
            events.emplace_back(Side::Buy, kHighestBid, market_qty_dist(rng), ts);
        }
    }

    return events;
}

void seed_local_book(LocalOrderBook &book) {
    for (Price price = kLowestBid; price <= kHighestBid; ++price) {
        book.on_event(MarketEvent{Side::Buy, price, 100, 0});
    }

    for (Price price = kLowestAsk; price <= kHighestAsk; ++price) {
        book.on_event(MarketEvent{Side::Sell, price, 100, 0});
    }
}

std::uint64_t elapsed_ns(const Clock::time_point &start, const Clock::time_point &end) {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

int main() {
    const auto events = make_events(kNumEvents);

    LocalOrderBook local_book(kMinPrice, kMaxPrice);
    seed_local_book(local_book);

    SimpleStrategy strategy(kMaxSpread, kStrategyQty);
    RiskCheck risk_check(kMaxOrderQty, kMaxPosition, kMaxOrders);

    SimulatedOrderGateway gateway(1'000'000);

    LatencyStats stats;
    stats.reserve(events.size());

    std::size_t events_processed = 0;
    std::size_t book_update_failed = 0;
    std::size_t orders_no_intent = 0;
    std::size_t orders_generated = 0;
    Quantity order_qty_generated = 0;
    std::size_t orders_rejected_by_risk_check = 0;
    std::size_t orders_accepted_by_risk_check = 0;
    std::size_t orders_rejected_by_engine = 0;

    for (const auto &event : events) {
        const auto start = Clock::now();

        bool book_ok = local_book.on_event(event);

        if (!book_ok) {
            const auto end = Clock::now();
            stats.add(elapsed_ns(start, end));
            ++book_update_failed;
            continue;
        }

        ++events_processed;

        const auto maybe_intent = strategy.on_book_update(local_book);
        if (!maybe_intent.has_value()) {
            const auto end = Clock::now();
            stats.add(elapsed_ns(start, end));
            ++orders_no_intent;
            continue;
        }

        ++orders_generated;

        const OrderIntent &intent = *maybe_intent;
        order_qty_generated += intent.qty;

        if (!risk_check.allow(intent)) {
            const auto end = Clock::now();
            stats.add(elapsed_ns(start, end));

            ++orders_rejected_by_risk_check;
            continue;
        }

        ++orders_accepted_by_risk_check;

        const bool submitted = gateway.submit(intent);

        if (submitted) {
            risk_check.record_order(intent);
        } else {
            ++orders_rejected_by_engine;
        }

        const auto end = Clock::now();
        stats.add(elapsed_ns(start, end));
    }

    std::cout << "Tick-to-order profiling results:\n";
    std::cout << "  events_processed: " << events_processed << "\n";
    std::cout << "  book_update_failed: " << book_update_failed << "\n";
    std::cout << "  orders_no_intent: " << orders_no_intent << "\n";
    std::cout << "  orders_generated: " << orders_generated << "\n";
    std::cout << "  order_qty_generated: " << order_qty_generated << "\n";
    std::cout << "  orders_rejected_by_risk_check: " << orders_rejected_by_risk_check << "\n";
    std::cout << "  orders_accepted_by_risk_check: " << orders_accepted_by_risk_check << "\n";
    std::cout << "  orders_submitted_by_gateway: " << gateway.submitted_count() << "\n";
    std::cout << "  orders_rejected_by_engine: " << orders_rejected_by_engine << "\n";
    std::cout << "  risk_position: " << risk_check.position() << "\n";
    std::cout << "  risk_orders_sent: " << risk_check.orders_sent() << "\n";

    std::cout << "\n  latency_stats:\n";
    stats.print();
    return 0;
}
