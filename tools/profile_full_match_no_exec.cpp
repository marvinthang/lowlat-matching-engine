#include "execution_sink.hpp"
#include "fixed_clob.hpp"

#include <iostream>
#include <chrono>

int main() {
    constexpr std::size_t n = 1'000'000;
    constexpr Price price = 10000;

    FixedClob<1 << 20, FastOrderIdHash> book(8000, 12000);
    NullExecutionSink dummy_executions;

    for (std::size_t i = 0; i < n; ++i) {
        OrderId ask_id = static_cast<OrderId>(i + 1);
        Quantity qty = static_cast<Quantity>(i % 100 + 1);

        bool ok = book.submit_limit_order(ask_id, Side::Sell, price, qty, dummy_executions);

        if (!ok) {
            std::cerr << "failed to preload ask " << ask_id << '\n';
            return 1;
        }
    }

    NullExecutionSink executions;
    executions.reserve(n);

    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < n; ++i) {
        OrderId buy_id = static_cast<OrderId>(n + i + 1);
        Quantity qty = static_cast<Quantity>(i % 100 + 1);

        bool ok = book.submit_limit_order(buy_id, Side::Buy, price, qty, executions);

        if (!ok) {
            std::cerr << "failed to submit buy " << buy_id << '\n';
            return 1;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "executions=" << executions.size() << "\n";
    std::cout << "active_orders=" << book.active_orders() << "\n";
    std::cout << "elapsed_ms=" << elapsed_ns / 1000000.0 << "\n";
    std::cout << "ns_per_match=" << static_cast<double>(elapsed_ns) / static_cast<double>(n) << "\n";

    if (executions.size() != 0) {
        std::cerr << "wrong execution count\n";
        return 1;
    }

    if (!book.empty()) {
        std::cerr << "book should be empty\n";
        return 1;
    }

    return 0;
}
