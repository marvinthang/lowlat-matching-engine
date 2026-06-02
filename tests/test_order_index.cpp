#include "order_index.hpp"

#include <cassert>
#include <iostream>

void test_empty_table() {
    FixedOrderIndex<8> index;

    assert(index.empty());
    assert(index.size() == 0);
    assert(index.capacity() == 8);
    assert(!index.find(1).has_value());
    assert(!index.contains(1));
}

void test_insert_find_one() {
    FixedOrderIndex<8> index;

    assert(index.insert(100, 3));
    assert(index.size() == 1);
    assert(index.contains(100));

    auto idx = index.find(100);
    assert(idx.has_value());
    assert(*idx == 3);
}

void test_duplicate_insert_fails() {
    FixedOrderIndex<8> index;

    assert(index.insert(100, 3));
    assert(!index.insert(100, 4));

    auto idx = index.find(100);
    assert(idx.has_value());
    assert(*idx == 3);
    assert(index.size() == 1);
}

void test_erase_existing() {
    FixedOrderIndex<8> index;

    assert(index.insert(100, 3));
    assert(index.erase(100));

    assert(index.empty());
    assert(!index.find(100).has_value());
    assert(!index.contains(100));
}

void test_erase_missing_fails() {
    FixedOrderIndex<8> index;

    assert(!index.erase(100));
    assert(index.empty());
}

void test_insert_after_erase_reuses_deleted_slot() {
    FixedOrderIndex<8> index;

    assert(index.insert(100, 3));
    assert(index.erase(100));

    assert(index.insert(200, 5));
    auto idx = index.find(200);

    assert(idx.has_value());
    assert(*idx == 5);
    assert(index.size() == 1);
}

void test_collision_chain() {
    FixedOrderIndex<8> index;

    assert(index.insert(1, 10));
    assert(index.insert(9, 20));
    assert(index.insert(17, 30));

    assert(index.find(1).value() == 10);
    assert(index.find(9).value() == 20);
    assert(index.find(17).value() == 30);
}

void test_erase_middle_of_probe_chain() {
    FixedOrderIndex<8> index;

    assert(index.insert(1, 10));
    assert(index.insert(9, 20));
    assert(index.insert(17, 30));

    assert(index.erase(9));

    assert(index.find(1).value() == 10);
    assert(!index.find(9).has_value());
    assert(index.find(17).value() == 30);
}

void test_fill_table() {
    FixedOrderIndex<4> index;

    assert(index.insert(1, 1));
    assert(index.insert(2, 2));
    assert(index.insert(3, 3));
    assert(index.insert(4, 4));

    assert(index.full());
    assert(index.size() == 4);

    assert(!index.insert(5, 5));
}

void test_clear() {
    FixedOrderIndex<8> index;

    assert(index.insert(1, 10));
    assert(index.insert(2, 20));

    index.clear();

    assert(index.empty());
    assert(!index.find(1).has_value());
    assert(!index.find(2).has_value());

    assert(index.insert(3, 30));
    assert(index.find(3).value() == 30);
}

int main() {
    test_empty_table();
    test_insert_find_one();
    test_duplicate_insert_fails();
    test_erase_existing();
    test_erase_missing_fails();
    test_insert_after_erase_reuses_deleted_slot();
    test_collision_chain();
    test_erase_middle_of_probe_chain();
    test_fill_table();
    test_clear();

    std::cout << "OrderIndex tests passed\n";
}
