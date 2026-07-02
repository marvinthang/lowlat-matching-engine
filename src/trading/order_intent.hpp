#pragma once

#include "core/types.hpp"

struct OrderIntent {
    Price price{0};
    Quantity qty{0};
    Side side{Side::Buy};
};
