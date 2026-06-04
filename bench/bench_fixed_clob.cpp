#include "workload.hpp"
#include "fixed_clob.hpp"

#include <benchmark/benchmark.h>

static void BM_FixedClob_AddOnly(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto commands = make_add_only_commands(n);

    for (auto _ : state) {
        FixedClob<1 << 20> book(8000, 12000);

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(
                book.add_order(command.order_id, command.side, command.price, command.qty));
        }

        benchmark::DoNotOptimize(book.empty());
    }

    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_FixedClob_AddThenCancel(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto commands = make_add_only_commands(n);

    for (auto _ : state) {
        FixedClob<1 << 20> book(8000, 12000);

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(
                book.add_order(command.order_id, command.side, command.price, command.qty));
        }

        for (const auto &command : commands) {
            benchmark::DoNotOptimize(book.cancel_order(command.order_id));
        }

        benchmark::DoNotOptimize(book.empty());
    }

    state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_FixedClob_AddOnly)->RangeMultiplier(10)->Range(1'000, 1'000'000);
BENCHMARK(BM_FixedClob_AddThenCancel)->RangeMultiplier(10)->Range(1'000, 1'000'000);

BENCHMARK_MAIN();
