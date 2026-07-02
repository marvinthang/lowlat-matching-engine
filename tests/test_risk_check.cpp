#include "trading/risk_check.hpp"

#include <cassert>
#include <iostream>

OrderIntent intent(Side side, Quantity qty) {
    return OrderIntent{10000, qty, side};
}

void test_small_order_allowed() {
    RiskCheck risk(10, 20, 2);

    assert(risk.allow(intent(Side::Buy, 10)));
}

void test_large_order_rejected() {
    RiskCheck risk(10, 20, 2);

    assert(!risk.allow(intent(Side::Buy, 11)));
}

void test_position_limit_rejected() {
    RiskCheck risk(20, 20, 2);

    OrderIntent first = intent(Side::Buy, 20);
    risk.record_order(first);

    assert(risk.position() == 20);
    assert(!risk.allow(intent(Side::Buy, 1)));
}

void test_max_orders_rejected() {
    RiskCheck risk(10, 100, 2);

    risk.record_order(intent(Side::Buy, 1));
    risk.record_order(intent(Side::Buy, 1));

    assert(risk.orders_sent() == 2);
    assert(!risk.allow(intent(Side::Buy, 1)));
}

int main() {
    test_small_order_allowed();
    test_large_order_rejected();
    test_position_limit_rejected();
    test_max_orders_rejected();

    std::cout << "RiskCheck tests passed\n";
}
