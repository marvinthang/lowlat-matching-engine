#include "trading/local_order_book.hpp"
#include "trading/simple_strategy.hpp"

#include <cassert>
#include <iostream>

MarketEvent event(Side side, Price price, Quantity qty) {
    return MarketEvent{price, qty, side, 0};
}

void test_empty_book_no_intent() {
    LocalOrderBook book(9990, 10010);
    SimpleStrategy strategy(2, 5);

    assert(!strategy.on_book_update(book).has_value());
}

void test_only_bid_no_intent() {
    LocalOrderBook book(9990, 10010);
    SimpleStrategy strategy(2, 5);

    assert(book.on_event(event(Side::Buy, 10000, 10)));

    assert(!strategy.on_book_update(book).has_value());
}

void test_only_ask_no_intent() {
    LocalOrderBook book(9990, 10010);
    SimpleStrategy strategy(2, 5);

    assert(book.on_event(event(Side::Sell, 10005, 10)));

    assert(!strategy.on_book_update(book).has_value());
}

void test_wide_spread_no_intent() {
    LocalOrderBook book(9990, 10010);
    SimpleStrategy strategy(2, 5);

    assert(book.on_event(event(Side::Buy, 10000, 10)));
    assert(book.on_event(event(Side::Sell, 10005, 10)));

    assert(!strategy.on_book_update(book).has_value());
}

void test_small_spread_produces_intent() {
    constexpr Quantity order_qty = 5;

    LocalOrderBook book(9990, 10010);
    SimpleStrategy strategy(2, order_qty);

    assert(book.on_event(event(Side::Buy, 10003, 10)));
    assert(book.on_event(event(Side::Sell, 10005, 10)));

    auto intent = strategy.on_book_update(book);
    assert(intent.has_value());
    assert(intent->side == Side::Buy);
    assert(intent->price == 10004);
    assert(intent->qty == order_qty);
}

int main() {
    test_empty_book_no_intent();
    test_only_bid_no_intent();
    test_only_ask_no_intent();
    test_wide_spread_no_intent();
    test_small_spread_produces_intent();

    std::cout << "SimpleStrategy tests passed\n";
}
