#pragma once

#include "core/types.hpp"

struct MarketEvent {
    Side side{Side::Buy};
    Price price{0};
    Quantity qty{0};
    Timestamp timestamp{0};
};
