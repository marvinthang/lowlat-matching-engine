#include "fixed_clob.hpp"

#include <cassert>
#include <iostream>

void test_empty_book() {
    FixedClob<8> book(9000, 11000);

    assert(book.empty());
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(book.active_orders() == 0);
    assert(book.available_slots() == 8);
}

void test_add_one_buy() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 50));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10000);
    assert(bid->second == 50);

    assert(book.quantity_at(Side::Buy, 10000) == 50);
    assert(book.order_count_at(Side::Buy, 10000) == 1);
    assert(book.front_order_id(Side::Buy, 10000).value() == 1);
    assert(book.back_order_id(Side::Buy, 10000).value() == 1);
}

void test_add_one_sell() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(2, Side::Sell, 10001, 70));

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 10001);
    assert(ask->second == 70);

    assert(book.quantity_at(Side::Sell, 10001) == 70);
    assert(book.order_count_at(Side::Sell, 10001) == 1);
}

void test_best_bid_ask_ordering() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 9999, 20));
    assert(book.add_order(3, Side::Buy, 10002, 30));

    assert(book.add_order(4, Side::Sell, 10005, 10));
    assert(book.add_order(5, Side::Sell, 10003, 20));
    assert(book.add_order(6, Side::Sell, 10006, 30));

    auto bid = book.best_bid();
    auto ask = book.best_ask();

    assert(bid.has_value());
    assert(ask.has_value());

    assert(bid->first == 10002);
    assert(bid->second == 30);

    assert(ask->first == 10003);
    assert(ask->second == 20);
}

void test_fifo_same_price() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10000, 20));
    assert(book.add_order(3, Side::Buy, 10000, 30));

    assert(book.order_count_at(Side::Buy, 10000) == 3);
    assert(book.quantity_at(Side::Buy, 10000) == 60);

    assert(book.front_order_id(Side::Buy, 10000).value() == 1);
    assert(book.back_order_id(Side::Buy, 10000).value() == 3);
}

void test_cancel_only_order_at_level() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.cancel_order(1));

    assert(book.empty());
    assert(!book.best_bid().has_value());
    assert(book.quantity_at(Side::Buy, 10000) == 0);
    assert(book.order_count_at(Side::Buy, 10000) == 0);
    assert(!book.front_order_id(Side::Buy, 10000).has_value());
    assert(!book.back_order_id(Side::Buy, 10000).has_value());
}

void test_cancel_head_of_level() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10000, 20));
    assert(book.add_order(3, Side::Buy, 10000, 30));

    assert(book.cancel_order(1));

    assert(book.order_count_at(Side::Buy, 10000) == 2);
    assert(book.quantity_at(Side::Buy, 10000) == 50);
    assert(book.front_order_id(Side::Buy, 10000).value() == 2);
    assert(book.back_order_id(Side::Buy, 10000).value() == 3);
}

void test_cancel_tail_of_level() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10000, 20));
    assert(book.add_order(3, Side::Buy, 10000, 30));

    assert(book.cancel_order(3));

    assert(book.order_count_at(Side::Buy, 10000) == 2);
    assert(book.quantity_at(Side::Buy, 10000) == 30);
    assert(book.front_order_id(Side::Buy, 10000).value() == 1);
    assert(book.back_order_id(Side::Buy, 10000).value() == 2);
}

void test_cancel_middle_of_level() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10000, 20));
    assert(book.add_order(3, Side::Buy, 10000, 30));

    assert(book.cancel_order(2));

    assert(book.order_count_at(Side::Buy, 10000) == 2);
    assert(book.quantity_at(Side::Buy, 10000) == 40);
    assert(book.front_order_id(Side::Buy, 10000).value() == 1);
    assert(book.back_order_id(Side::Buy, 10000).value() == 3);
}

void test_cancel_best_refreshes() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10002, 20));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10002);

    assert(book.cancel_order(2));

    bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10000);
    assert(bid->second == 10);
}

void test_duplicate_order_rejected() {
    FixedClob<8> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(!book.add_order(1, Side::Buy, 10001, 20));

    assert(book.active_orders() == 1);
}

void test_invalid_order_rejected() {
    FixedClob<8> book(9000, 11000);

    assert(!book.add_order(1, Side::Buy, 8999, 10));
    assert(!book.add_order(2, Side::Buy, 11001, 10));
    assert(!book.add_order(3, Side::Buy, 10000, 0));

    assert(book.empty());
}

void test_pool_full() {
    FixedClob<2> book(9000, 11000);

    assert(book.add_order(1, Side::Buy, 10000, 10));
    assert(book.add_order(2, Side::Buy, 10001, 20));
    assert(!book.add_order(3, Side::Buy, 10002, 30));

    assert(book.active_orders() == 2);
    assert(book.available_slots() == 0);
}

int main() {
    test_empty_book();
    test_add_one_buy();
    test_add_one_sell();
    test_best_bid_ask_ordering();
    test_fifo_same_price();
    test_cancel_only_order_at_level();
    test_cancel_head_of_level();
    test_cancel_tail_of_level();
    test_cancel_middle_of_level();
    test_cancel_best_refreshes();
    test_duplicate_order_rejected();
    test_invalid_order_rejected();
    test_pool_full();

    std::cout << "FixedClob tests passed\n";
}
