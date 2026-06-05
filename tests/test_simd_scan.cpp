#include "simd_scan.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

template <std::size_t NumBlocks>
void check_same_block(const std::vector<std::uint32_t>& a, std::size_t start) {
    auto scalar = scalar_find_next_nonzero(a.data(), a.size(), start);
    auto avx2 = avx2_find_next_nonzero<NumBlocks>(a.data(), a.size(), start);

    assert(scalar == avx2);
}

void check_same_all_blocks(const std::vector<std::uint32_t>& a, std::size_t start) {
    check_same_block<1>(a, start);
    check_same_block<2>(a, start);
    check_same_block<3>(a, start);
    check_same_block<4>(a, start);
    check_same_block<5>(a, start);
    check_same_block<6>(a, start);
    check_same_block<7>(a, start);
    check_same_block<8>(a, start);
}

void test_all_zero() {
    std::vector<std::uint32_t> a(256, 0);

    for (std::size_t start : {0ULL, 1ULL, 7ULL, 8ULL, 31ULL, 32ULL, 63ULL, 64ULL, 127ULL, 255ULL, 256ULL}) {
        check_same_all_blocks(a, start);
    }

    assert(!avx2_find_next_nonzero<1>(a.data(), a.size(), 0).has_value());
    assert(!avx2_find_next_nonzero<8>(a.data(), a.size(), 0).has_value());
}

void test_first_element_nonzero() {
    std::vector<std::uint32_t> a(256, 0);
    a[0] = 1;

    check_same_all_blocks(a, 0);
    check_same_all_blocks(a, 1);

    auto pos = avx2_find_next_nonzero<8>(a.data(), a.size(), 0);
    assert(pos.has_value());
    assert(*pos == 0);
}

void test_positions_around_vector_boundaries() {
    std::vector<std::size_t> positions = {
        1, 7, 8, 9,
        15, 16, 17,
        31, 32, 33,
        63, 64, 65,
        127, 128, 129,
        191, 192, 193,
        255
    };

    for (std::size_t pos : positions) {
        std::vector<std::uint32_t> a(256, 0);
        a[pos] = 123;

        check_same_all_blocks(a, 0);
        check_same_all_blocks(a, pos);

        if (pos > 0) {
            check_same_all_blocks(a, pos - 1);
        }

        if (pos + 1 <= a.size()) {
            check_same_all_blocks(a, pos + 1);
        }
    }
}

void test_step_boundaries_for_each_block_size() {
    for (std::size_t blocks = 1; blocks <= 8; ++blocks) {
        std::size_t step = blocks * 8;

        for (std::size_t pos : {step - 1, step, step + 1, 2 * step - 1, 2 * step, 2 * step + 1}) {
            std::vector<std::uint32_t> a(256, 0);
            if (pos >= a.size()) {
                continue;
            }

            a[pos] = 777;

            check_same_all_blocks(a, 0);
            check_same_all_blocks(a, pos);

            if (pos > 0) {
                check_same_all_blocks(a, pos - 1);
            }
        }
    }
}

void test_start_offset_skips_earlier_values() {
    std::vector<std::uint32_t> a(256, 0);
    a[3] = 10;
    a[8] = 20;
    a[65] = 30;
    a[130] = 40;

    check_same_all_blocks(a, 0);
    check_same_all_blocks(a, 4);
    check_same_all_blocks(a, 9);
    check_same_all_blocks(a, 66);
    check_same_all_blocks(a, 131);

    auto pos = avx2_find_next_nonzero<8>(a.data(), a.size(), 4);
    assert(pos.has_value());
    assert(*pos == 8);

    pos = avx2_find_next_nonzero<8>(a.data(), a.size(), 66);
    assert(pos.has_value());
    assert(*pos == 130);

    pos = avx2_find_next_nonzero<8>(a.data(), a.size(), 131);
    assert(!pos.has_value());
}

void test_multiple_nonzero_returns_first() {
    std::vector<std::uint32_t> a(256, 0);
    a[20] = 1;
    a[21] = 2;
    a[80] = 3;
    a[200] = 4;

    check_same_all_blocks(a, 0);
    check_same_all_blocks(a, 20);
    check_same_all_blocks(a, 21);
    check_same_all_blocks(a, 22);
    check_same_all_blocks(a, 81);
    check_same_all_blocks(a, 201);

    auto pos = avx2_find_next_nonzero<4>(a.data(), a.size(), 0);
    assert(pos.has_value());
    assert(*pos == 20);

    pos = avx2_find_next_nonzero<4>(a.data(), a.size(), 21);
    assert(pos.has_value());
    assert(*pos == 21);

    pos = avx2_find_next_nonzero<4>(a.data(), a.size(), 22);
    assert(pos.has_value());
    assert(*pos == 80);
}

void test_tail_less_than_vector_width() {
    for (std::size_t n : {1ULL, 2ULL, 3ULL, 7ULL, 8ULL, 9ULL, 10ULL, 15ULL, 17ULL}) {
        std::vector<std::uint32_t> a(n, 0);
        a[n - 1] = 99;

        check_same_all_blocks(a, 0);
        check_same_all_blocks(a, n - 1);

        auto pos = avx2_find_next_nonzero<8>(a.data(), a.size(), n - 1);
        assert(pos.has_value());
        assert(*pos == n - 1);
    }
}

void test_empty_vector() {
    std::vector<std::uint32_t> a;

    check_same_all_blocks(a, 0);

    auto pos = avx2_find_next_nonzero<1>(a.data(), a.size(), 0);
    assert(!pos.has_value());

    pos = avx2_find_next_nonzero<8>(a.data(), a.size(), 0);
    assert(!pos.has_value());
}

void test_exhaustive_small_patterns() {
    for (std::size_t n = 1; n <= 257; ++n) {
        for (std::size_t pos = 0; pos < n; ++pos) {
            std::vector<std::uint32_t> a(n, 0);
            a[pos] = static_cast<std::uint32_t>(pos + 1);

            check_same_all_blocks(a, 0);
            check_same_all_blocks(a, pos);

            if (pos > 0) {
                check_same_all_blocks(a, pos - 1);
            }

            if (pos + 1 <= n) {
                check_same_all_blocks(a, pos + 1);
            }
        }
    }
}

int main() {
    test_all_zero();
    test_first_element_nonzero();
    test_positions_around_vector_boundaries();
    test_step_boundaries_for_each_block_size();
    test_start_offset_skips_earlier_values();
    test_multiple_nonzero_returns_first();
    test_tail_less_than_vector_width();
    test_empty_vector();
    test_exhaustive_small_patterns();

    std::cout << "SIMD scan tests passed\n";
}
