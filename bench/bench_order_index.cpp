#include "types.hpp"
#include "order_index.hpp"

#include <benchmark/benchmark.h>

#include <type_traits>
#include <vector>
#include <unordered_map>

static std::vector<OrderId> make_sequential_ids(std::size_t n) {
    std::vector<OrderId> ids;
    ids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        ids.push_back(static_cast<OrderId>(i + 1));
    }
    return ids;
}

static std::vector<OrderId> make_bad_low_bit_ids(std::size_t n) {
    std::vector<OrderId> ids;
    ids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        ids.push_back(static_cast<OrderId>((i + 1) << 10));
    }
    return ids;
}

constexpr std::size_t index_capacity = 1 << 21;
using FastIndex = FixedOrderIndex<index_capacity, FastOrderIdHash>;
using RobustIndex = FixedOrderIndex<index_capacity, RobustOrderIdHash>;
using UnorderedMapIndex = std::unordered_map<OrderId, OrderIndex>;

template <class Index>
static void reserve_index(Index &index, std::size_t n) {
    if constexpr (std::is_same_v<Index, UnorderedMapIndex>) {
        index.reserve(2 * n);
    }
}

template <class Index>
static bool insert_order(Index &index, OrderId id) {
    if constexpr (std::is_same_v<Index, UnorderedMapIndex>) {
        return index.insert({id, static_cast<OrderIndex>(1)}).second;
    } else {
        return index.insert(id, static_cast<OrderIndex>(1));
    }
}

enum class IdPattern {
    Sequential,
    BadLowBits,
};

template <IdPattern Pattern>
static std::vector<OrderId> make_ids(std::size_t n) {
    if constexpr (Pattern == IdPattern::Sequential) {
        return make_sequential_ids(n);
    }
    return make_bad_low_bit_ids(n);
}

template <class Index, IdPattern Pattern>
static void BM_Index_Insert(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto ids = make_ids<Pattern>(n);

    for (auto _ : state) {
        Index index;
        reserve_index(index, n);

        for (const auto &id : ids) {
            benchmark::DoNotOptimize(insert_order(index, id));
        }

        benchmark::DoNotOptimize(index.size());
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class Index, IdPattern Pattern>
static void BM_Index_Find(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto ids = make_ids<Pattern>(n);

    for (auto _ : state) {
        state.PauseTiming();
        Index index;
        reserve_index(index, n);

        for (const auto &id : ids) {
            benchmark::DoNotOptimize(insert_order(index, id));
        }

        state.ResumeTiming();

        for (const auto &id : ids) {
            benchmark::DoNotOptimize(index.find(id));
        }

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

template <class Index, IdPattern Pattern>
static void BM_Index_Erase(benchmark::State &state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto ids = make_ids<Pattern>(n);

    for (auto _ : state) {
        state.PauseTiming();
        Index index;
        reserve_index(index, n);

        for (const auto &id : ids) {
            benchmark::DoNotOptimize(insert_order(index, id));
        }

        state.ResumeTiming();

        for (const auto &id : ids) {
            benchmark::DoNotOptimize(index.erase(id));
        }

        state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations() * n);
}

#define DEFINE_INDEX_BENCHMARKS(prefix, index_type)                        \
    static void BM_##prefix##_Sequential_Insert(benchmark::State &state) { \
        BM_Index_Insert<index_type, IdPattern::Sequential>(state);         \
    }                                                                      \
    static void BM_##prefix##_Sequential_Find(benchmark::State &state) {   \
        BM_Index_Find<index_type, IdPattern::Sequential>(state);           \
    }                                                                      \
    static void BM_##prefix##_Sequential_Erase(benchmark::State &state) {  \
        BM_Index_Erase<index_type, IdPattern::Sequential>(state);          \
    }                                                                      \
    static void BM_##prefix##_BadLowBits_Insert(benchmark::State &state) { \
        BM_Index_Insert<index_type, IdPattern::BadLowBits>(state);         \
    }                                                                      \
    static void BM_##prefix##_BadLowBits_Find(benchmark::State &state) {   \
        BM_Index_Find<index_type, IdPattern::BadLowBits>(state);           \
    }                                                                      \
    static void BM_##prefix##_BadLowBits_Erase(benchmark::State &state) {  \
        BM_Index_Erase<index_type, IdPattern::BadLowBits>(state);          \
    }

DEFINE_INDEX_BENCHMARKS(FastIndex, FastIndex)
DEFINE_INDEX_BENCHMARKS(RobustIndex, RobustIndex)
DEFINE_INDEX_BENCHMARKS(UnorderedMap, UnorderedMapIndex)

#define REGISTER_INDEX_BENCHMARKS(prefix)                                                     \
    BENCHMARK(BM_##prefix##_Sequential_Insert)->RangeMultiplier(10)->Range(1'000, 1'000'000); \
    BENCHMARK(BM_##prefix##_Sequential_Find)->RangeMultiplier(10)->Range(1'000, 1'000'000);   \
    BENCHMARK(BM_##prefix##_Sequential_Erase)->RangeMultiplier(10)->Range(1'000, 1'000'000);  \
    BENCHMARK(BM_##prefix##_BadLowBits_Insert)->RangeMultiplier(10)->Range(1'000, 1'000'000); \
    BENCHMARK(BM_##prefix##_BadLowBits_Find)->RangeMultiplier(10)->Range(1'000, 1'000'000);   \
    BENCHMARK(BM_##prefix##_BadLowBits_Erase)->RangeMultiplier(10)->Range(1'000, 1'000'000);

REGISTER_INDEX_BENCHMARKS(FastIndex)
REGISTER_INDEX_BENCHMARKS(RobustIndex)
REGISTER_INDEX_BENCHMARKS(UnorderedMap)

BENCHMARK_MAIN();
