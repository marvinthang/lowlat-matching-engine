#include "order_pool.hpp"

#include <cassert>
#include <iostream>

void test_initial_state() {
    OrderPool<4> pool;

    assert(pool.capacity() == 4);
    assert(pool.available() == 4);
    assert(pool.used() == 0);
    assert(pool.empty());
    assert(!pool.full());
}

void test_allocate_one() {
    OrderPool<4> pool;

    auto idx = pool.allocate();
    assert(idx.has_value());

    assert(pool.available() == 3);
    assert(pool.used() == 1);
    assert(!pool.empty());
    assert(!pool.full());

    auto &order = pool.get(*idx);
    order.order_id = 123;
    order.price = 10000;
    order.qty = 50;
    order.side = Side::Buy;

    const auto &read = pool.get(*idx);
    assert(read.order_id == 123);
    assert(read.price == 10000);
    assert(read.qty == 50);
    assert(read.side == Side::Buy);
}

void test_allocate_until_full() {
    OrderPool<4> pool;

    auto a = pool.allocate();
    auto b = pool.allocate();
    auto c = pool.allocate();
    auto d = pool.allocate();

    assert(a.has_value());
    assert(b.has_value());
    assert(c.has_value());
    assert(d.has_value());

    assert(pool.available() == 0);
    assert(pool.used() == 4);
    assert(pool.full());

    auto e = pool.allocate();
    assert(!e.has_value());
}

void test_release_and_reuse() {
    OrderPool<4> pool;

    auto idx = pool.allocate();
    assert(idx.has_value());

    auto &written = pool.get(*idx);
    written.order_id = 999;
    written.price = 10000;
    written.qty = 50;
    written.side = Side::Buy;
    written.next_idx = 1;
    written.prev_idx = 2;

    pool.release(*idx);

    assert(pool.available() == 4);
    assert(pool.used() == 0);
    assert(pool.empty());

    auto again = pool.allocate();
    assert(again.has_value());
    assert(*again == *idx);

    const auto &order = pool.get(*again);
    assert(order.order_id == 999);
    assert(order.price == 10000);
    assert(order.qty == 50);
    assert(order.side == Side::Buy);
    assert(order.next_idx == 1);
    assert(order.prev_idx == 2);
}

void test_release_middle_reuse_lifo() {
    OrderPool<4> pool;

    auto a = pool.allocate();
    auto b = pool.allocate();
    auto c = pool.allocate();

    assert(a.has_value());
    assert(b.has_value());
    assert(c.has_value());

    pool.release(*b);

    auto d = pool.allocate();
    assert(d.has_value());
    assert(*d == *b);
}

int main() {
    test_initial_state();
    test_allocate_one();
    test_allocate_until_full();
    test_release_and_reuse();
    test_release_middle_reuse_lifo();

    std::cout << "OrderPool tests passed\n";

    return 0;
}
