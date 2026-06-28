#include "order_command.hpp"
#include "workload.hpp"
#include "spsc_ring.hpp"
#include "latency_stats.hpp"
#include "fixed_clob.hpp"
#include "execution_sink.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

struct TimedOrderCommand {
    OrderCommand cmd{};
    std::uint64_t sent_ns{0};
};

static std::uint64_t now_ns() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

struct ProfileResult {
    std::size_t capacity{};
    double elapsed_ms{};
    std::uint64_t queue_p50_ns{};
    std::uint64_t queue_p99_ns{};
    std::uint64_t service_p50_ns{};
    std::uint64_t service_p99_ns{};
    std::uint64_t end_to_end_p99_ns{};
};

template <std::size_t QueueCapacity>
static ProfileResult run_profile(const std::vector<OrderCommand> &commands, std::size_t expected_executions) {
    SpscRing<TimedOrderCommand, QueueCapacity> queue;
    FixedClob<1 << 20> book(8000, 12000);

    ExecutionBuffer executions;
    executions.reserve(expected_executions);

    LatencyStats queue_latency;
    LatencyStats service_latency;
    LatencyStats end_to_end_latency;

    queue_latency.reserve(commands.size());
    service_latency.reserve(commands.size());
    end_to_end_latency.reserve(commands.size());

    std::atomic<bool> start{false};

    std::thread producer([&] {
        while (!start.load(std::memory_order_acquire)) {}

        std::size_t i = 0;
        while (i < commands.size()) {
            TimedOrderCommand timed{
                commands[i],
                now_ns(),
            };

            if (queue.push(timed)) {
                ++i;
            }
        }
    });

    std::thread consumer([&] {
        while (!start.load(std::memory_order_acquire)) {}

        TimedOrderCommand timed{};
        std::size_t processed = 0;

        while (processed < commands.size()) {
            if (!queue.pop(timed)) {
                continue;
            }

            std::uint64_t received_ns = now_ns();
            queue_latency.add(received_ns - timed.sent_ns);

            const auto &cmd = timed.cmd;

            if (cmd.type == CommandType::AddLimit) {
                bool ok = book.submit_limit_order(cmd.order_id, cmd.side, cmd.price, cmd.qty, executions);

                if (!ok) {
                    std::cerr << "submit failed for order_id=" << cmd.order_id << "\n";
                    std::exit(1);
                }
            } else if (cmd.type == CommandType::Cancel) {
                bool ok = book.cancel_order(cmd.order_id);

                if (!ok) {
                    std::cerr << "cancel failed for order_id=" << cmd.order_id << "\n";
                    std::exit(1);
                }
            }

            std::uint64_t done_ns = now_ns();
            service_latency.add(done_ns - received_ns);
            end_to_end_latency.add(done_ns - timed.sent_ns);

            ++processed;
        }
    });

    std::uint64_t start_ns = now_ns();

    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    std::uint64_t end_ns = now_ns();

    if (executions.size() != expected_executions) {
        std::cerr << "expected executions size to be " << expected_executions << " but got "
                  << executions.size() << "\n";
        std::exit(1);
    }

    if (!book.empty()) {
        std::cerr << "expected book to be empty but it has " << book.active_orders() << " active orders\n";
        std::exit(1);
    }

    return ProfileResult{
        .capacity = QueueCapacity,
        .elapsed_ms = static_cast<double>(end_ns - start_ns) / 1e6,
        .queue_p50_ns = queue_latency.percentile(0.5),
        .queue_p99_ns = queue_latency.percentile(0.99),
        .service_p50_ns = service_latency.percentile(0.5),
        .service_p99_ns = service_latency.percentile(0.99),
        .end_to_end_p99_ns = end_to_end_latency.percentile(0.99),
    };
}

static void print_result(const ProfileResult &result) {
    std::cout << std::setw(10) << result.capacity
              << std::setw(14) << std::fixed << std::setprecision(3) << result.elapsed_ms
              << std::setw(15) << result.queue_p50_ns
              << std::setw(15) << result.queue_p99_ns
              << std::setw(16) << result.service_p50_ns
              << std::setw(16) << result.service_p99_ns
              << std::setw(18) << result.end_to_end_p99_ns << '\n';
}

int main() {
    constexpr std::size_t n = 100'000;
    const auto commands = make_full_match_commands(n);

    std::cout << "commands=" << commands.size() << "\n";
    std::cout << "expected_executions=" << n << "\n";
    std::cout << std::setw(10) << "capacity"
              << std::setw(14) << "elapsed_ms"
              << std::setw(15) << "queue_p50_ns"
              << std::setw(15) << "queue_p99_ns"
              << std::setw(16) << "service_p50_ns"
              << std::setw(16) << "service_p99_ns"
              << std::setw(18) << "end_to_end_p99_ns" << '\n';

    print_result(run_profile<64>(commands, n));
    print_result(run_profile<256>(commands, n));
    print_result(run_profile<1024>(commands, n));
    print_result(run_profile<4096>(commands, n));
    print_result(run_profile<65536>(commands, n));
    print_result(run_profile<1048576>(commands, n));

    return 0;
}
