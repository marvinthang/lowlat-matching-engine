#include "trading/local_order_book.hpp"

#include <cassert>
#include <iostream>

MarketEvent event(Side side, Price price, Quantity qty) {
    return MarketEvent{price, qty, side, 0};
}

void test_empty_book() {
    LocalOrderBook book(100, 110);

    assert(!book.has_bid());
    assert(!book.has_ask());
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(!book.spread().has_value());
    assert(book.quantity_at(Side::Buy, 105) == 0);
    assert(book.quantity_at(Side::Sell, 105) == 0);
}

void test_add_one_bid() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 105, 25)));

    assert(book.has_bid());
    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 105);
    assert(bid->second == 25);
    assert(book.quantity_at(Side::Buy, 105) == 25);
    assert(!book.has_ask());
}

void test_add_one_ask() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Sell, 106, 40)));

    assert(book.has_ask());
    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 106);
    assert(ask->second == 40);
    assert(book.quantity_at(Side::Sell, 106) == 40);
    assert(!book.has_bid());
}

void test_best_bid_and_ask_ordering() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 104, 10)));
    assert(book.on_event(event(Side::Buy, 103, 20)));
    assert(book.on_event(event(Side::Buy, 105, 30)));

    assert(book.on_event(event(Side::Sell, 108, 10)));
    assert(book.on_event(event(Side::Sell, 107, 20)));
    assert(book.on_event(event(Side::Sell, 109, 30)));

    auto bid = book.best_bid();
    auto ask = book.best_ask();

    assert(bid.has_value());
    assert(ask.has_value());

    assert(bid->first == 105);
    assert(bid->second == 30);

    assert(ask->first == 107);
    assert(ask->second == 20);
}

void test_update_existing_level_quantity() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 105, 25)));
    assert(book.on_event(event(Side::Buy, 105, 60)));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 105);
    assert(bid->second == 60);
    assert(book.quantity_at(Side::Buy, 105) == 60);
}

void test_remove_non_best_level_keeps_best() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 104, 10)));
    assert(book.on_event(event(Side::Buy, 106, 20)));
    assert(book.on_event(event(Side::Buy, 104, 0)));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 106);
    assert(bid->second == 20);
    assert(book.quantity_at(Side::Buy, 104) == 0);
}

void test_remove_best_bid_refreshes_lower() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 102, 10)));
    assert(book.on_event(event(Side::Buy, 105, 20)));
    assert(book.on_event(event(Side::Buy, 104, 30)));

    assert(book.on_event(event(Side::Buy, 105, 0)));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 104);
    assert(bid->second == 30);
}

void test_remove_best_ask_refreshes_higher() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Sell, 108, 10)));
    assert(book.on_event(event(Side::Sell, 105, 20)));
    assert(book.on_event(event(Side::Sell, 106, 30)));

    assert(book.on_event(event(Side::Sell, 105, 0)));

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 106);
    assert(ask->second == 30);
}

void test_remove_last_levels_clears_best() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 105, 10)));
    assert(book.on_event(event(Side::Sell, 106, 20)));

    assert(book.on_event(event(Side::Buy, 105, 0)));
    assert(book.on_event(event(Side::Sell, 106, 0)));

    assert(!book.has_bid());
    assert(!book.has_ask());
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(!book.spread().has_value());
}

void test_boundaries_are_valid_prices() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 100, 10)));
    assert(book.on_event(event(Side::Sell, 110, 20)));

    auto bid = book.best_bid();
    auto ask = book.best_ask();
    assert(bid.has_value());
    assert(ask.has_value());
    assert(bid->first == 100);
    assert(bid->second == 10);
    assert(ask->first == 110);
    assert(ask->second == 20);
}

void test_invalid_prices_are_rejected() {
    LocalOrderBook book(100, 110);

    assert(book.on_event(event(Side::Buy, 105, 15)));
    assert(!book.on_event(event(Side::Buy, 99, 25)));
    assert(!book.on_event(event(Side::Sell, 111, 35)));

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 105);
    assert(bid->second == 15);

    assert(!book.has_ask());
    assert(book.quantity_at(Side::Buy, 99) == 0);
    assert(book.quantity_at(Side::Sell, 111) == 0);
}

void test_spread_requires_both_sides() {
    LocalOrderBook book(100, 110);

    assert(!book.spread().has_value());

    assert(book.on_event(event(Side::Buy, 104, 10)));
    assert(!book.spread().has_value());

    assert(book.on_event(event(Side::Sell, 107, 20)));
    auto spread = book.spread();
    assert(spread.has_value());
    assert(*spread == 3);
}

int main() {
    test_empty_book();
    test_add_one_bid();
    test_add_one_ask();
    test_best_bid_and_ask_ordering();
    test_update_existing_level_quantity();
    test_remove_non_best_level_keeps_best();
    test_remove_best_bid_refreshes_lower();
    test_remove_best_ask_refreshes_higher();
    test_remove_last_levels_clears_best();
    test_boundaries_are_valid_prices();
    test_invalid_prices_are_rejected();
    test_spread_requires_both_sides();

    std::cout << "LocalOrderBook tests passed\n";
}
