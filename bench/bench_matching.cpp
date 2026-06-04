#include "workload.hpp"
#include "execution_sink.hpp"
#include "fixed_clob.hpp"

#include <benchmark/benchmark.h>

template <class OrderIdHash>
static void run_matching_add_only(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto commands = make_add_only_commands(n);

    for (auto _ : state) {
        FixedClob<1 << 20, OrderIdHash> book(8000, 12000);
        ExecutionBuffer executions;
        executions.reserve(n);

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(book.submit_limit_order(
                command.order_id, command.side, command.price, command.qty, executions));
        }

        benchmark::DoNotOptimize(book.best_bid());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class OrderIdHash>
static void run_matching_full_match(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));

    const auto commands = make_full_match_commands(n);

    for (auto _ : state) {
        FixedClob<1 << 20, OrderIdHash> book(8000, 12000);
        ExecutionBuffer executions;
        executions.reserve(n);

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(book.submit_limit_order(
                command.order_id, command.side, command.price, command.qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * 2 * n);
}

template <class OrderIdHash>
static void run_matching_sweep_ask_levels(benchmark::State &state) {
    const auto levels = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
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

        {
            OrderId buy_id = static_cast<OrderId>(levels + 1);
            Price price = static_cast<Price>(10000 + levels);
            Quantity qty = static_cast<Quantity>(levels);

            benchmark::DoNotOptimize(
                book.submit_limit_order(buy_id, Side::Buy, price, qty, executions));
        }

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * levels);
}

static void BM_Direct_Fast_AddOnly_RawExec(benchmark::State &state) {
    run_matching_add_only<FastOrderIdHash>(state);
}

static void BM_Direct_Fast_FullMatch_RawExec(benchmark::State &state) {
    run_matching_full_match<FastOrderIdHash>(state);
}

static void BM_Direct_Fast_SweepAskLevels_RawExec(benchmark::State &state) {
    run_matching_sweep_ask_levels<FastOrderIdHash>(state);
}

static void BM_Direct_Robust_AddOnly_RawExec(benchmark::State &state) {
    run_matching_add_only<RobustOrderIdHash>(state);
}

static void BM_Direct_Robust_FullMatch_RawExec(benchmark::State &state) {
    run_matching_full_match<RobustOrderIdHash>(state);
}

static void BM_Direct_Robust_SweepAskLevels_RawExec(benchmark::State &state) {
    run_matching_sweep_ask_levels<RobustOrderIdHash>(state);
}

BENCHMARK(BM_Direct_Fast_AddOnly_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Direct_Fast_FullMatch_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Direct_Fast_SweepAskLevels_RawExec)->RangeMultiplier(10)->Range(10, 10'000);

BENCHMARK(BM_Direct_Robust_AddOnly_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Direct_Robust_FullMatch_RawExec)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_Direct_Robust_SweepAskLevels_RawExec)->RangeMultiplier(10)->Range(10, 10'000);

BENCHMARK_MAIN();
