#include "simd_scan.hpp"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

static std::vector<std::uint32_t> make_all_zero(std::size_t n) {
    return std::vector<std::uint32_t>(n, 0);
}

static std::vector<std::uint32_t> make_one_nonzero(std::size_t n, std::size_t pos) {
    std::vector<std::uint32_t> a(n, 0);
    if (pos < n) {
        a[pos] = 1;
    }
    return a;
}

static std::vector<std::uint32_t> make_sparse_every(std::size_t n, std::size_t step) {
    std::vector<std::uint32_t> a(n, 0);
    for (std::size_t i = 0; i < n; i += step) {
        a[i] = 1;
    }
    return a;
}

static void BM_Scalar_AllZero(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_all_zero(n);

    for (auto _ : state) {
        auto pos = scalar_find_next_nonzero(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <std::size_t NumBlocks>
static void BM_AVX2_AllZero(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_all_zero(n);

    for (auto _ : state) {
        auto pos = avx2_find_next_nonzero<NumBlocks>(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Scalar_HitFar(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_one_nonzero(n, n - 1);

    for (auto _ : state) {
        auto pos = scalar_find_next_nonzero(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <std::size_t NumBlocks>
static void BM_AVX2_HitFar(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_one_nonzero(n, n - 1);

    for (auto _ : state) {
        auto pos = avx2_find_next_nonzero<NumBlocks>(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Scalar_HitNear(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_one_nonzero(n, 8);

    for (auto _ : state) {
        auto pos = scalar_find_next_nonzero(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * 9);
}

template <std::size_t NumBlocks>
static void BM_AVX2_HitNear(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_one_nonzero(n, 8);

    for (auto _ : state) {
        auto pos = avx2_find_next_nonzero<NumBlocks>(a.data(), a.size(), 0);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * 9);
}

static void BM_Scalar_SparseEvery64(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_sparse_every(n, 64);

    for (auto _ : state) {
        auto pos = scalar_find_next_nonzero(a.data(), a.size(), 1);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * 64);
}

template <std::size_t NumBlocks>
static void BM_AVX2_SparseEvery64(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_sparse_every(n, 64);

    for (auto _ : state) {
        auto pos = avx2_find_next_nonzero<NumBlocks>(a.data(), a.size(), 1);
        benchmark::DoNotOptimize(pos);
    }

    state.SetItemsProcessed(state.iterations() * 64);
}

BENCHMARK(BM_Scalar_AllZero)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_AllZero<1>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_AllZero<2>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_AllZero<4>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_AllZero<8>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();

BENCHMARK(BM_Scalar_HitFar)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitFar<2>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitFar<1>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitFar<4>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitFar<8>)->RangeMultiplier(4)->Range(1024, 1 << 20)->UseRealTime();

BENCHMARK(BM_Scalar_HitNear)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitNear<1>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitNear<2>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitNear<4>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_HitNear<8>)->Arg(1 << 20)->UseRealTime();

BENCHMARK(BM_Scalar_SparseEvery64)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_SparseEvery64<1>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_SparseEvery64<2>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_SparseEvery64<4>)->Arg(1 << 20)->UseRealTime();
BENCHMARK(BM_AVX2_SparseEvery64<8>)->Arg(1 << 20)->UseRealTime();

BENCHMARK_MAIN();
