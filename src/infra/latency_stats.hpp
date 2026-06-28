#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

class LatencyStats {
public:
    void reserve(std::size_t n) {
        samples_.reserve(n);
    }

    void add(std::uint64_t ns) {
        samples_.push_back(ns);
        sorted_dirty_ = true;
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

        ensure_sorted();
        return percentile_sorted(sorted_samples_, p);
    }

    void print() const {
        if (samples_.empty()) {
            std::cout << "no latency samples\n";
            return;
        }

        ensure_sorted();

        std::uint64_t sum =
            std::accumulate(sorted_samples_.begin(), sorted_samples_.end(), std::uint64_t(0));

        double mean = static_cast<double>(sum) / static_cast<double>(sorted_samples_.size());

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "count=" << sorted_samples_.size() << "\n";
        std::cout << "mean_ns=" << mean << "\n";
        std::cout << "min_ns=" << sorted_samples_.front() << "\n";
        std::cout << "p50_ns=" << percentile_sorted(sorted_samples_, 0.5) << "\n";
        std::cout << "p90_ns=" << percentile_sorted(sorted_samples_, 0.9) << "\n";
        std::cout << "p99_ns=" << percentile_sorted(sorted_samples_, 0.99) << "\n";
        std::cout << "p999_ns=" << percentile_sorted(sorted_samples_, 0.999) << "\n";
        std::cout << "max_ns=" << sorted_samples_.back() << "\n";
    }

private:
    void ensure_sorted() const {
        if (!sorted_dirty_ && sorted_samples_.size() == samples_.size()) {
            return;
        }

        sorted_samples_ = samples_;
        std::sort(sorted_samples_.begin(), sorted_samples_.end());
        sorted_dirty_ = false;
    }

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
    mutable std::vector<std::uint64_t> sorted_samples_;
    mutable bool sorted_dirty_{true};
};
