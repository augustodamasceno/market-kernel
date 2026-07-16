/* Market Kernel : Tick Struct
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_tick.hpp
 * @brief Lightweight aggregate view of a single market tick.
 */

#pragma once

#include <cstdint>

#include "mk_side.h"

namespace marketkernel {

/**
 * @brief Lightweight, aggregate view of a single tick composed from MarketData SoA arrays.
 *
 * @details Not stored; instances are temporary views returned by operator[] or at().
 * All fields are copies, not references, so the instance is valid even if the
 * source MarketData/MarketDataView is destructed.
 *
 * @tparam Num Numeric type for price, quantity, and orders.
 */
template<typename Num>
struct Tick
{
    uint64_t timestamp; ///< Nanosecond epoch timestamp.
    Side     side;      ///< Buy or sell.
    uint8_t  level;     ///< Orderbook depth (0 = trade, >= 1 = book level).
    Num      price;     ///< Price at this tick.
    Num      quantity;  ///< Quantity transacted.
    Num      orders;    ///< Number of orders at this price level.
};

}  // namespace marketkernel
