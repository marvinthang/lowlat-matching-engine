#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iomanip>

class LatencyStats {
public:
    void reserve(std::size_t n) {
        samples_.reserve(n);
    }

    void add(std::uint64_t ns) {
        samples_.push_back(ns);
    }

    std::size_t size() const {
        return samples_.size();
    }

    bool empty() const {
        return samples_.empty();
    }

    std::uint64_t percentile(double p) const {
        if (samples_.empty()) {
            return 0;
        }

        std::vector<std::uint64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        return percentile_sorted(sorted, p);
    }

    void print() const {
        if (samples_.empty()) {
            std::cout << "no latency samples\n";
            return;
        }

        std::vector<std::uint64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        std::uint64_t sum = std::accumulate(sorted.begin(), sorted.end(), std::uint64_t(0));

        double mean = static_cast<double>(sum) / static_cast<double>(sorted.size());

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "count=" << sorted.size() << "\n";
        std::cout << "mean_ns=" << mean << "\n";
        std::cout << "min_ns=" << sorted.front() << "\n";
        std::cout << "p50_ns=" << percentile_sorted(sorted, 0.5) << "\n";
        std::cout << "p90_ns=" << percentile_sorted(sorted, 0.9) << "\n";
        std::cout << "p99_ns=" << percentile_sorted(sorted, 0.99) << "\n";
        std::cout << "p999_ns=" << percentile_sorted(sorted, 0.999) << "\n";
        std::cout << "max_ns=" << sorted.back() << "\n";
    }

private:
    static std::uint64_t percentile_sorted(const std::vector<std::uint64_t> &sorted, double p) {
        if (sorted.empty()) {
            return 0;
        }

        double pos = p * static_cast<double>(sorted.size() - 1);
        auto idx = static_cast<std::size_t>(pos);

        if (idx >= sorted.size() - 1) {
            return sorted.back();
        }

        return sorted[idx];
    }

    std::vector<std::uint64_t> samples_;
};
