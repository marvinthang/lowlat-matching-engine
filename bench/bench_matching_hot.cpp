#include "market_order.hpp"
#include "fixed_clob.hpp"
#include "execution_sink.hpp"

#include <benchmark/benchmark.h>

template <class OrderIdHash>
static void BM_Matching_Hot_AddOnly(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto orders = make_non_crossing_buys(n);

    for (auto _ : state) {
        state.PauseTiming();
        FixedClob<1 << 20, OrderIdHash> book(8000, 12000);
        std::vector<Execution> executions;
        state.ResumeTiming();

        for (const auto &order : orders) {
            benchmark::DoNotOptimize(book.submit_limit_order(order.order_id, order.side,
                                                             order.price, order.qty, executions));
        }

        benchmark::DoNotOptimize(book.best_bid());
        benchmark::DoNotOptimize(executions.size());

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class OrderIdHash, class ExecutionSink>
static void BM_Matching_Hot_FullMatchOneLevel(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        FixedClob<1 << 20, OrderIdHash> book(8000, 12000);
        NullExecutionSink preload_executions;

        for (std::size_t i = 0; i < n; ++i) {
            OrderId ask_id = static_cast<OrderId>(i + 1);
            Quantity qty = static_cast<Quantity>(i % 100 + 1);

            benchmark::DoNotOptimize(
                book.submit_limit_order(ask_id, Side::Sell, 10000, qty, preload_executions));
        }

        ExecutionSink executions;
        executions.reserve(n);

        state.ResumeTiming();

        for (std::size_t i = 0; i < n; ++i) {
            OrderId buy_id = static_cast<OrderId>(n + i + 1);
            Quantity qty = static_cast<Quantity>(i % 100 + 1);

            benchmark::DoNotOptimize(
                book.submit_limit_order(buy_id, Side::Buy, 10000, qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class OrderIdHash>
static void BM_Matching_Hot_SweepAskLevels(benchmark::State &state) {
    const auto levels = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        FixedClob<1 << 15, OrderIdHash> book(8000, 25000);
        std::vector<Execution> executions;
        executions.reserve(levels);

        for (std::size_t i = 0; i < levels; ++i) {
            OrderId ask_id = static_cast<OrderId>(i + 1);
            Price price = static_cast<Price>(10001 + i);
            Quantity qty = static_cast<Quantity>(1);

            benchmark::DoNotOptimize(
                book.submit_limit_order(ask_id, Side::Sell, price, qty, executions));
        }

        state.ResumeTiming();

        {
            OrderId buy_id = static_cast<OrderId>(levels + 1);
            Price price = static_cast<Price>(10000 + levels);
            Quantity qty = static_cast<Quantity>(levels);

            benchmark::DoNotOptimize(
                book.submit_limit_order(buy_id, Side::Buy, price, qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * levels);
}

static void BM_Matching_Hot_Fast_AddOnly(benchmark::State &state) {
    BM_Matching_Hot_AddOnly<FastOrderIdHash>(state);
}

static void BM_Matching_Hot_Fast_SweepAskLevels(benchmark::State &state) {
    BM_Matching_Hot_SweepAskLevels<FastOrderIdHash>(state);
}

static void BM_Matching_Hot_Robust_AddOnly(benchmark::State &state) {
    BM_Matching_Hot_AddOnly<RobustOrderIdHash>(state);
}

static void BM_Matching_Hot_Robust_SweepAskLevels(benchmark::State &state) {
    BM_Matching_Hot_SweepAskLevels<RobustOrderIdHash>(state);
}


static void BM_Matching_Hot_Fast_FullMatchOneLevel_NullExec(benchmark::State &state) {
    BM_Matching_Hot_FullMatchOneLevel<FastOrderIdHash, NullExecutionSink>(state);
}

static void BM_Matching_Hot_Robust_FullMatchOneLevel_NullExec(benchmark::State &state) {
    BM_Matching_Hot_FullMatchOneLevel<RobustOrderIdHash, NullExecutionSink>(state);
}

static void BM_Matching_Hot_Fast_FullMatchOneLevel_RawExec(benchmark::State &state) {
    BM_Matching_Hot_FullMatchOneLevel<FastOrderIdHash, ExecutionBuffer>(state);
}

static void BM_Matching_Hot_Fast_FullMatchOneLevel_VecExec(benchmark::State &state) {
    BM_Matching_Hot_FullMatchOneLevel<FastOrderIdHash, std::vector<Execution>>(state);
}

// BENCHMARK(BM_Matching_Hot_Fast_AddOnly)->RangeMultiplier(10)->Range(1'000, 1'000'000);
// BENCHMARK(BM_Matching_Hot_Fast_FullMatchOneLevel_NullExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
// BENCHMARK(BM_Matching_Hot_Fast_SweepAskLevels)->RangeMultiplier(10)->Range(10, 10'000);

// BENCHMARK(BM_Matching_Hot_Robust_AddOnly)->RangeMultiplier(10)->Range(1'000, 1'000'000);
// BENCHMARK(BM_Matching_Hot_Robust_FullMatchOneLevel_NullExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
// BENCHMARK(BM_Matching_Hot_Robust_SweepAskLevels)->RangeMultiplier(10)->Range(10, 10'000);

BENCHMARK(BM_Matching_Hot_Fast_FullMatchOneLevel_NullExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Matching_Hot_Fast_FullMatchOneLevel_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Matching_Hot_Fast_FullMatchOneLevel_VecExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);

BENCHMARK_MAIN();
