#include "execution_sink.hpp"
#include "fixed_clob.hpp"
#include "workload.hpp"
#include "spsc_ring.hpp"
#include "thread_affinity.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

constexpr std::size_t queue_capacity = 1 << 20;
constexpr std::size_t batch_size = 32;

static void BM_Pipeline_Fast_AddOnly_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_add_only_commands(n);

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;
        FixedClob<queue_capacity, FastOrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                if (queue.push(commands[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            OrderCommand command{};
            std::size_t processed = 0;

            while (processed < commands.size()) {
                if (!queue.pop(command)) {
                    continue;
                }

                if (command.type == CommandType::AddLimit) {
                    benchmark::DoNotOptimize(book.submit_limit_order(
                        command.order_id, command.side, command.price, command.qty, executions));
                } else if (command.type == CommandType::Cancel) {
                    benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                }

                ++processed;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

static void BM_Pipeline_Fast_FullMatch_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;
        FixedClob<queue_capacity, FastOrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                if (queue.push(commands[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            OrderCommand command{};
            std::size_t processed = 0;

            while (processed < commands.size()) {
                if (!queue.pop(command)) {
                    continue;
                }

                if (command.type == CommandType::AddLimit) {
                    benchmark::DoNotOptimize(book.submit_limit_order(
                        command.order_id, command.side, command.price, command.qty, executions));
                } else if (command.type == CommandType::Cancel) {
                    benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                }

                ++processed;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

static void BM_Pipeline_Fast_FullMatch_IndexQueue_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    for (auto _ : state) {
        SpscRing<std::uint32_t, queue_capacity> queue;
        FixedClob<queue_capacity, FastOrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::uint32_t i = 0;
            while (i < commands.size()) {
                if (queue.push(i)) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::uint32_t i = 0;
            std::size_t processed = 0;

            while (processed < commands.size()) {
                if (!queue.pop(i)) {
                    continue;
                }

                const auto &command = commands[i];

                if (command.type == CommandType::AddLimit) {
                    benchmark::DoNotOptimize(book.submit_limit_order(
                        command.order_id, command.side, command.price, command.qty, executions));
                } else if (command.type == CommandType::Cancel) {
                    benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                }

                ++processed;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

template <std::size_t BatchSize = batch_size, class OrderIdHash = FastOrderIdHash>
static void BM_Pipeline_FullMatch_BatchPop_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    std::array<OrderCommand, BatchSize> command_batch;

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;
        FixedClob<queue_capacity, OrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                if (queue.push(commands[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t processed = 0;

            while (processed < commands.size()) {
                std::size_t batch_count = queue.pop_many(command_batch.data(), BatchSize);

                for (std::size_t i = 0; i < batch_count; ++i) {
                    const auto &command = command_batch[i];

                    if (command.type == CommandType::AddLimit) {
                        benchmark::DoNotOptimize(book.submit_limit_order(
                            command.order_id, command.side, command.price, command.qty, executions));
                    } else if (command.type == CommandType::Cancel) {
                        benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                    }
                }

                processed += batch_count;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

template <std::size_t BatchSize = batch_size>
static void BM_Pipeline_FullMatch_QueueOnly(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                if (queue.push(commands[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::array<OrderCommand, BatchSize> command_batch;
            std::size_t processed = 0;

            while (processed < commands.size()) {
                std::size_t batch_count = queue.pop_many(command_batch.data(), BatchSize);
                if (batch_count == 0) {
                    continue;
                }

                for (std::size_t i = 0; i < batch_count; ++i) {
                    benchmark::DoNotOptimize(command_batch[i]);
                }

                processed += batch_count;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

static void BM_Pipeline_Uint64_QueueOnly(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::vector<std::uint64_t> items(n);
    for (std::size_t i = 0; i < n; ++i) {
        items[i] = static_cast<std::uint64_t>(i + 1);
    }

    for (auto _ : state) {
        SpscRing<std::uint64_t, queue_capacity> queue;

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < items.size()) {
                if (queue.push(items[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::uint64_t x = 0;
            std::size_t processed = 0;

            while (processed < items.size()) {
                if (!queue.pop(x)) {
                    continue;
                }
                benchmark::DoNotOptimize(x);
                ++processed;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();
    }

    state.SetItemsProcessed(state.iterations() * items.size());
}

static void BM_Pipeline_FullMatch_IndexQueueOnly(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    for (auto _ : state) {
        SpscRing<std::uint32_t, queue_capacity> queue;

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::uint32_t i = 0;
            while (i < commands.size()) {
                if (queue.push(i)) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::uint32_t i = 0;
            std::size_t processed = 0;

            while (processed < commands.size()) {
                if (!queue.pop(i)) {
                    continue;
                }
                benchmark::DoNotOptimize(commands[i]);
                ++processed;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

template <std::size_t BatchSize = batch_size, class OrderIdHash = FastOrderIdHash>
static void BM_Pipeline_FullMatch_BatchPushPop_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    std::array<OrderCommand, BatchSize> consumer_batch;

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;
        FixedClob<queue_capacity, OrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                std::size_t max_count = std::min(BatchSize, commands.size() - i);

                std::size_t pushed = queue.push_many(commands.data() + i, max_count);
                i += pushed;
            }
        });

        std::thread consumer([&] {
            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t processed = 0;

            while (processed < commands.size()) {
                std::size_t popped = queue.pop_many(consumer_batch.data(), BatchSize);

                for (std::size_t i = 0; i < popped; ++i) {
                    const auto &command = consumer_batch[i];

                    if (command.type == CommandType::AddLimit) {
                        benchmark::DoNotOptimize(book.submit_limit_order(
                            command.order_id, command.side, command.price, command.qty, executions));
                    } else if (command.type == CommandType::Cancel) {
                        benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                    }
                }

                processed += popped;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

template <std::size_t BatchSize = batch_size, class OrderIdHash = FastOrderIdHash>
static void BM_Pipeline_FullMatch_BatchPop_Pinned_RawExec(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    auto commands = make_full_match_commands(n);

    std::array<OrderCommand, BatchSize> command_batch;

    for (auto _ : state) {
        SpscRing<OrderCommand, queue_capacity> queue;
        FixedClob<queue_capacity, OrderIdHash> book(8000, 12000);

        ExecutionBuffer executions;
        executions.reserve(n);

        std::atomic<bool> start{false};

        std::thread producer([&] {
            benchmark::DoNotOptimize(pin_current_thread_to_cpu(4));

            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t i = 0;
            while (i < commands.size()) {
                if (queue.push(commands[i])) {
                    ++i;
                }
            }
        });

        std::thread consumer([&] {
            benchmark::DoNotOptimize(pin_current_thread_to_cpu(6));

            while (!start.load(std::memory_order_acquire)) {
            }

            std::size_t processed = 0;

            while (processed < commands.size()) {
                std::size_t batch_count = queue.pop_many(command_batch.data(), BatchSize);

                for (std::size_t i = 0; i < batch_count; ++i) {
                    const auto &command = command_batch[i];

                    if (command.type == CommandType::AddLimit) {
                        benchmark::DoNotOptimize(book.submit_limit_order(
                            command.order_id, command.side, command.price, command.qty, executions));
                    } else if (command.type == CommandType::Cancel) {
                        benchmark::DoNotOptimize(book.cancel_order(command.order_id));
                    }
                }

                processed += batch_count;
            }
        });

        start.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        benchmark::DoNotOptimize(book.empty());
        benchmark::DoNotOptimize(executions.size());
    }

    state.SetItemsProcessed(state.iterations() * commands.size());
}

BENCHMARK(BM_Pipeline_Fast_AddOnly_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();
BENCHMARK(BM_Pipeline_Fast_FullMatch_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();

BENCHMARK(BM_Pipeline_FullMatch_QueueOnly<32>)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();

BENCHMARK(BM_Pipeline_Uint64_QueueOnly)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();
BENCHMARK(BM_Pipeline_FullMatch_IndexQueueOnly)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();
BENCHMARK(BM_Pipeline_Fast_FullMatch_IndexQueue_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();

BENCHMARK(BM_Pipeline_FullMatch_BatchPop_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();

BENCHMARK(BM_Pipeline_FullMatch_BatchPop_Pinned_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();

BENCHMARK(BM_Pipeline_FullMatch_BatchPushPop_RawExec)
    ->RangeMultiplier(10)
    ->Range(1'000, 1'000'000)
    ->UseRealTime();


BENCHMARK_MAIN();
