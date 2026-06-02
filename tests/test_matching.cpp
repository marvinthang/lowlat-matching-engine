#include "fixed_clob.hpp"
#include "execution.hpp"

#include <cassert>
#include <iostream>
#include <vector>

void test_non_crossing_buy_rests() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.submit_limit_order(1, Side::Buy, 10000, 5, executions));

    assert(executions.empty());

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10000);
    assert(bid->second == 5);

    assert(!book.best_ask().has_value());

    assert(book.active_orders() == 1);
    assert(book.quantity_at(Side::Buy, 10000) == 5);
    assert(book.order_count_at(Side::Buy, 10000) == 1);
    assert(book.front_order_id(Side::Buy, 10000).value() == 1);
}

void test_buy_fully_fills_one_ask() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Sell, 10001, 5));

    assert(book.submit_limit_order(99, Side::Buy, 10001, 5, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10001);
    assert(executions[0].qty == 5);

    assert(book.empty());
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(book.quantity_at(Side::Sell, 10001) == 0);
    assert(book.order_count_at(Side::Sell, 10001) == 0);
}

void test_buy_partially_fills_resting_ask() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Sell, 10001, 10));

    assert(book.submit_limit_order(99, Side::Buy, 10001, 4, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10001);
    assert(executions[0].qty == 4);

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 10001);
    assert(ask->second == 6);

    assert(!book.best_bid().has_value());

    assert(book.active_orders() == 1);
    assert(book.quantity_at(Side::Sell, 10001) == 6);
    assert(book.order_count_at(Side::Sell, 10001) == 1);
    assert(book.front_order_id(Side::Sell, 10001).value() == 10);
}

void test_buy_consumes_ask_and_rests_remaining_bid() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Sell, 10001, 4));

    assert(book.submit_limit_order(99, Side::Buy, 10001, 10, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10001);
    assert(executions[0].qty == 4);

    assert(!book.best_ask().has_value());

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10001);
    assert(bid->second == 6);

    assert(book.active_orders() == 1);
    assert(book.quantity_at(Side::Buy, 10001) == 6);
    assert(book.order_count_at(Side::Buy, 10001) == 1);
    assert(book.front_order_id(Side::Buy, 10001).value() == 99);
}

void test_buy_fifo_same_price() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Sell, 10001, 5));
    assert(book.add_order(11, Side::Sell, 10001, 5));

    assert(book.submit_limit_order(99, Side::Buy, 10001, 7, executions));

    assert(executions.size() == 2);

    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10001);
    assert(executions[0].qty == 5);

    assert(executions[1].incoming_order_id == 99);
    assert(executions[1].resting_order_id == 11);
    assert(executions[1].price == 10001);
    assert(executions[1].qty == 2);

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 10001);
    assert(ask->second == 3);

    assert(book.quantity_at(Side::Sell, 10001) == 3);
    assert(book.order_count_at(Side::Sell, 10001) == 1);
    assert(book.front_order_id(Side::Sell, 10001).value() == 11);
    assert(book.back_order_id(Side::Sell, 10001).value() == 11);
}

void test_buy_sweeps_multiple_ask_levels_and_rests() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Sell, 10001, 3));
    assert(book.add_order(11, Side::Sell, 10002, 4));
    assert(book.add_order(12, Side::Sell, 10004, 8));

    assert(book.submit_limit_order(99, Side::Buy, 10002, 10, executions));

    assert(executions.size() == 2);

    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10001);
    assert(executions[0].qty == 3);

    assert(executions[1].incoming_order_id == 99);
    assert(executions[1].resting_order_id == 11);
    assert(executions[1].price == 10002);
    assert(executions[1].qty == 4);

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 10004);
    assert(ask->second == 8);

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10002);
    assert(bid->second == 3);

    assert(book.quantity_at(Side::Buy, 10002) == 3);
    assert(book.front_order_id(Side::Buy, 10002).value() == 99);
}

void test_sell_fully_fills_one_bid() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Buy, 10000, 5));

    assert(book.submit_limit_order(99, Side::Sell, 10000, 5, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10000);
    assert(executions[0].qty == 5);

    assert(book.empty());
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
}

void test_sell_partially_fills_resting_bid() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Buy, 10000, 10));

    assert(book.submit_limit_order(99, Side::Sell, 10000, 4, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10000);
    assert(executions[0].qty == 4);

    auto bid = book.best_bid();
    assert(bid.has_value());
    assert(bid->first == 10000);
    assert(bid->second == 6);

    assert(!book.best_ask().has_value());

    assert(book.quantity_at(Side::Buy, 10000) == 6);
    assert(book.order_count_at(Side::Buy, 10000) == 1);
    assert(book.front_order_id(Side::Buy, 10000).value() == 10);
}

void test_sell_consumes_bid_and_rests_remaining_ask() {
    FixedClob<16> book(9000, 11000);
    std::vector<Execution> executions;

    assert(book.add_order(10, Side::Buy, 10000, 4));

    assert(book.submit_limit_order(99, Side::Sell, 10000, 10, executions));

    assert(executions.size() == 1);
    assert(executions[0].incoming_order_id == 99);
    assert(executions[0].resting_order_id == 10);
    assert(executions[0].price == 10000);
    assert(executions[0].qty == 4);

    assert(!book.best_bid().has_value());

    auto ask = book.best_ask();
    assert(ask.has_value());
    assert(ask->first == 10000);
    assert(ask->second == 6);

    assert(book.quantity_at(Side::Sell, 10000) == 6);
    assert(book.order_count_at(Side::Sell, 10000) == 1);
    assert(book.front_order_id(Side::Sell, 10000).value() == 99);
}

int main() {
    test_non_crossing_buy_rests();
    test_buy_fully_fills_one_ask();
    test_buy_partially_fills_resting_ask();
    test_buy_consumes_ask_and_rests_remaining_bid();
    test_buy_fifo_same_price();
    test_buy_sweeps_multiple_ask_levels_and_rests();

    test_sell_fully_fills_one_bid();
    test_sell_partially_fills_resting_bid();
    test_sell_consumes_bid_and_rests_remaining_ask();

    std::cout << "Matching tests passed\n";
}
