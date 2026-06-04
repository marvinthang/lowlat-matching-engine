#include "workload.hpp"
#include "fixed_clob.hpp"
#include "execution_sink.hpp"

#include <benchmark/benchmark.h>

template <class OrderIdHash>
[[maybe_unused]] static void run_matching_hot_add_only(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto commands = make_non_crossing_buys_commands(n);

    for (auto _ : state) {
        state.PauseTiming();
        FixedClob<1 << 20, OrderIdHash> book(8000, 12000);
        ExecutionBuffer executions;
        executions.reserve(n);
        state.ResumeTiming();

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(book.submit_limit_order(
                command.order_id, command.side, command.price, command.qty, executions));
        }

        benchmark::DoNotOptimize(book.best_bid());
        benchmark::DoNotOptimize(executions.size());

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class OrderIdHash, class ExecutionSink>
static void run_matching_hot_full_match(benchmark::State &state) {
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
[[maybe_unused]] static void run_matching_hot_sweep_ask_levels(benchmark::State &state) {
    const auto levels = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        FixedClob<1 << 15, OrderIdHash> book(8000, 25000);
        ExecutionBuffer executions;
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

static void BM_DirectHot_Fast_FullMatch_NullExec(benchmark::State &state) {
    run_matching_hot_full_match<FastOrderIdHash, NullExecutionSink>(state);
}

static void BM_DirectHot_Fast_FullMatch_RawExec(benchmark::State &state) {
    run_matching_hot_full_match<FastOrderIdHash, ExecutionBuffer>(state);
}

static void BM_DirectHot_Fast_FullMatch_VecExec(benchmark::State &state) {
    run_matching_hot_full_match<FastOrderIdHash, std::vector<Execution>>(state);
}

BENCHMARK(BM_DirectHot_Fast_FullMatch_NullExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_DirectHot_Fast_FullMatch_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_DirectHot_Fast_FullMatch_VecExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);

BENCHMARK_MAIN();
