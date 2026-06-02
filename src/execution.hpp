#pragma once

#include "types.hpp"

#include <iostream>

struct Execution {
    OrderId incoming_order_id{0};
    OrderId resting_order_id{0};
    Price price{0};
    Quantity qty{0};

    friend std::ostream& operator<<(std::ostream &out, const Execution &exec) {
        out << "Execution(incoming_order_id=" << exec.incoming_order_id
            << ", resting_order_id=" << exec.resting_order_id
            << ", price=" << exec.price
            << ", qty=" << exec.qty << ")";
        return out;
    }
};
