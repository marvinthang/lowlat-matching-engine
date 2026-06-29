#pragma once

#include "core/types.hpp"

struct MarketEvent {
    Price price{0};
    Quantity qty{0};
    Side side{Side::Buy};
    Timestamp timestamp{0};
};
