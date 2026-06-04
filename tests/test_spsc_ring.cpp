#include "spsc_ring.hpp"

#include <cassert>
#include <iostream>
#include <thread>

void test_empty() {
    SpscRing<int, 4> q;
    int x = -1;
    assert(!q.pop(x));
}

void test_push_pop_one() {
    SpscRing<int, 4> q;
    int x = -1;

    assert(q.push(42));
    assert(q.pop(x));
    assert(x == 42);
    assert(!q.pop(x));
}

void test_fifo() {
    SpscRing<int, 4> q;
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
}

void test_full() {
    SpscRing<int, 4> q;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));
    assert(q.push(4));
    assert(!q.push(5));
}

void test_wrap_around() {
    SpscRing<int, 4> q;
    int x = -1;

    assert(q.push(1));
    assert(q.push(2));
    assert(q.push(3));
    assert(q.push(4));

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

void test_single_producer_single_consumer() {
    constexpr int n = 100000;
    SpscRing<int, 1024> q;

    std::thread producer([&] {
        for (int i = 1; i <= n;) {
            if (q.push(i)) {
                ++i;
            }
        }
    });

    std::thread consumer([&] {
        int expected = 1;
        int x = 0;

        while (expected <= n) {
            if (!q.pop(x)) {
                continue;
            }

            assert(x == expected);
            ++expected;
        }

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
    test_single_producer_single_consumer();

    std::cout << "SPSC tests passed\n";
}
