#pragma once

#include <cstddef>
#include <cstdint>
#include <bit>
#include <optional>
#include <immintrin.h>

inline std::optional<std::size_t> scalar_find_next_nonzero(const std::uint32_t *a, std::size_t n, std::size_t start = 0) {
    for (std::size_t i = start; i < n; ++i) {
        if (a[i] != 0) {
            return i;
        }
    }
    return std::nullopt;
}

template <std::size_t NumBlocks = 1>
inline std::optional<std::size_t> avx2_find_next_nonzero(const std::uint32_t *a, std::size_t n, std::size_t start = 0) {
    static_assert(NumBlocks > 0, "NumBlocks must be greater than 0");
    static_assert(NumBlocks <= 8, "NumBlocks must be less than or equal to 8");

    if (start >= n) {
        return std::nullopt;
    }

    constexpr std::size_t lanes = 8;
    constexpr std::size_t step = NumBlocks * lanes;
    constexpr uint64_t full = NumBlocks == 8 ? ~uint64_t(0) : ((uint64_t(1) << (NumBlocks * lanes)) - 1);

    const __m256i zero = _mm256_setzero_si256();
    std::size_t i = start;

    for (; i + step <= n; i += step) {
        uint64_t mask = 0;

        #pragma GCC unroll 8
        for (std::size_t b = 0; b < NumBlocks; ++b) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i_u*>(a + i + b * lanes));
            __m256i eq = _mm256_cmpeq_epi32(vec, zero);

            int zero_mask = _mm256_movemask_ps(_mm256_castsi256_ps(eq));

            mask |= static_cast<uint64_t>(zero_mask) << (b * lanes);
        }

        if (mask != full) {
            return i + static_cast<std::size_t>(std::countr_one(static_cast<uint64_t>(mask)));
        }
    }

    for (; i + lanes <= n; i += lanes) {
        __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i_u*>(a + i));
        __m256i eq = _mm256_cmpeq_epi32(vec, zero);

        int zero_mask = _mm256_movemask_ps(_mm256_castsi256_ps(eq));

        if (zero_mask != 0xFFu) {
            return i + static_cast<std::size_t>(std::countr_one(static_cast<unsigned int>(zero_mask)));
        }
    }

    for (; i < n; ++i) {
        if (a[i] != 0) {
            return i;
        }
    }

    return std::nullopt;
}
