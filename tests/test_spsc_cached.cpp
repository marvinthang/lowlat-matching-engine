#include "spsc_ring_cached.hpp"

#include <cassert>
#include <array>
#include <iostream>
#include <thread>
#include <vector>

void test_empty() {
    SpscRingCached<int, 4> q;

    int x = -1;
    assert(!q.pop(x));
}

void test_push_pop_one() {
    SpscRingCached<int, 4> q;

    int x = -1;

    assert(q.push(42));
    assert(q.pop(x));
    assert(x == 42);
    assert(!q.pop(x));
}

void test_fifo() {
    SpscRingCached<int, 4> q;

    int x = -1;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));

    assert(q.pop(x));
    assert(x == 1);

    assert(q.pop(x));
    assert(x == 2);

    assert(q.pop(x));
    assert(x == 3);

    assert(!q.pop(x));
}

void test_full() {
    SpscRingCached<int, 4> q;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));
    assert(q.push(4));
    assert(!q.push(5));
}

void test_wrap_around() {
    SpscRingCached<int, 4> q;

    int x = -1;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));
    assert(q.push(4));
    assert(!q.push(5));

    assert(q.pop(x));
    assert(x == 1);

    assert(q.pop(x));
    assert(x == 2);

    assert(q.push(5));
    assert(q.push(6));
    assert(!q.push(7));

    assert(q.pop(x));
    assert(x == 3);

    assert(q.pop(x));
    assert(x == 4);

    assert(q.pop(x));
    assert(x == 5);

    assert(q.pop(x));
    assert(x == 6);

    assert(!q.pop(x));
}

void test_pop_many_empty() {
    SpscRingCached<int, 8> q;

    int out[4]{};
    assert(q.pop_many(out, 4) == 0);
}

void test_pop_many_partial() {
    SpscRingCached<int, 8> q;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));

    int out[8]{};
    std::size_t cnt = q.pop_many(out, 8);

    assert(cnt == 3);
    assert(out[0] == 1);
    assert(out[1] == 2);
    assert(out[2] == 3);

    int x = -1;
    assert(!q.pop(x));
}

void test_pop_many_limited() {
    SpscRingCached<int, 8> q;

    for (int i = 1; i <= 6; ++i) {
        assert(q.push(i));
    }

    int out[3]{};
    std::size_t cnt = q.pop_many(out, 3);

    assert(cnt == 3);
    assert(out[0] == 1);
    assert(out[1] == 2);
    assert(out[2] == 3);

    int x = -1;

    assert(q.pop(x));
    assert(x == 4);

    assert(q.pop(x));
    assert(x == 5);

    assert(q.pop(x));
    assert(x == 6);

    assert(!q.pop(x));
}

void test_pop_many_wrap_around() {
    SpscRingCached<int, 8> q;

    for (int i = 1; i <= 8; ++i) {
        assert(q.push(i));
    }

    int x = -1;

    for (int i = 1; i <= 5; ++i) {
        assert(q.pop(x));
        assert(x == i);
    }

    assert(q.push(9));
    assert(q.push(10));
    assert(q.push(11));
    assert(q.push(12));
    assert(q.push(13));

    int out[8]{};
    std::size_t got = 0;

    while (got < 8) {
        std::size_t cnt = q.pop_many(out + got, 8 - got);
        assert(cnt > 0);
        got += cnt;
    }

    for (int i = 0; i < 8; ++i) {
        assert(out[i] == i + 6);
    }

    assert(!q.pop(x));
}

void test_threaded_fifo() {
    constexpr int n = 100000;

    SpscRingCached<int, 1024> q;
    std::vector<int> out(n);

    std::thread producer([&] {
        for (int i = 0; i < n;) {
            if (q.push(i)) {
                ++i;
            }
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < n;) {
            int x = -1;
            if (q.pop(x)) {
                out[i] = x;
                ++i;
            }
        }
    });

    producer.join();
    consumer.join();

    for (int i = 0; i < n; ++i) {
        assert(out[i] == i);
    }
}

void test_threaded_pop_many_fifo() {
    constexpr int n = 100000;
    constexpr std::size_t batch_size = 17;

    SpscRingCached<int, 1024> q;

    std::thread producer([&] {
        for (int i = 0; i < n;) {
            if (q.push(i)) {
                ++i;
            }
        }
    });

    std::thread consumer([&] {
        std::array<int, batch_size> batch{};
        int expected = 0;

        while (expected < n) {
            std::size_t count = q.pop_many(batch.data(), batch.size());
            if (count == 0) {
                continue;
            }

            for (std::size_t i = 0; i < count; ++i) {
                assert(batch[i] == expected);
                ++expected;
            }
        }

        int x = -1;
        assert(!q.pop(x));
    });

    producer.join();
    consumer.join();
}

int main() {
    test_empty();
    test_push_pop_one();
    test_fifo();
    test_full();
    test_wrap_around();

    test_pop_many_empty();
    test_pop_many_partial();
    test_pop_many_limited();
    test_pop_many_wrap_around();

    test_threaded_fifo();
    test_threaded_pop_many_fifo();

    std::cout << "SPSC cached tests passed\n";
    return 0;
}
