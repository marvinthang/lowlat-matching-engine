#include "market_order.hpp"
#include "fixed_clob.hpp"

#include <benchmark/benchmark.h>

static void BM_Matching_AddOnly(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto orders = make_non_crossing_buys(n);

    for (auto _ : state) {
        FixedClob<1 << 20> book(8000, 12000);
        std::vector<Execution> executions;

        for (const auto &order : orders) {
            benchmark::DoNotOptimize(book.submit_limit_order(order.order_id, order.side, order.price, order.qty, executions));
        }

        benchmark::DoNotOptimize(book.best_bid());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Matching_FullMatchOneLevel(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        FixedClob<1 << 20> book(8000, 12000);
        std::vector<Execution> executions;
        executions.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            OrderId ask_id = static_cast<OrderId>(i + 1);
            Quantity qty = static_cast<Quantity>(i % 100 + 1);

            benchmark::DoNotOptimize(book.submit_limit_order(ask_id, Side::Sell, 10000, qty, executions));
        }

        for (std::size_t i = 0; i < n; ++i) {
            OrderId buy_id = static_cast<OrderId>(n + i + 1);
            Quantity qty = static_cast<Quantity>(i % 100 + 1);

            benchmark::DoNotOptimize(book.submit_limit_order(buy_id, Side::Buy, 10000, qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * 2 * n);
}

static void BM_Matching_SweepAskLevels(benchmark::State& state) {
    const auto levels = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        FixedClob<1 << 15> book(8000, 25000);
        std::vector<Execution> executions;
        executions.reserve(levels);

        for (std::size_t i = 0; i < levels; ++i) {
            OrderId ask_id = static_cast<OrderId>(i + 1);
            Price price = static_cast<Price>(10001 + i);
            Quantity qty = static_cast<Quantity>(1);

            benchmark::DoNotOptimize(book.submit_limit_order(ask_id, Side::Sell, price, qty, executions));
        }

        {
            OrderId buy_id = static_cast<OrderId>(levels + 1);
            Price price = static_cast<Price>(10000 + levels);
            Quantity qty = static_cast<Quantity>(levels);

            benchmark::DoNotOptimize(book.submit_limit_order(buy_id, Side::Buy, price, qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * levels);
}

BENCHMARK(BM_Matching_AddOnly)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Matching_FullMatchOneLevel)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Matching_SweepAskLevels)->RangeMultiplier(10)->Range(10, 10'000);

BENCHMARK_MAIN();
