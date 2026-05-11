/* Market Kernel : MarketDataMode Enum
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_market_data_mode.h
 * @brief MarketDataMode enum and string conversion helper.
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace marketkernel {

/**
 * @brief Controls which fields are stored per tick in MarketData.
 *
 * @details
 * - ALL: stores every field (timestamp, side, level, price, quantity, orders);
 *        use for full orderbook replay or multi-purpose datasets.
 * - TRADE: trade ticks only; level field is ignored and not stored;
 *          use when only last-traded-price/quantity data is needed.
 * - LIQUIDITY: stores all fields, semantically scoped to orderbook
 *              liquidity updates (adds, modifies, deletes).
 * - LEVEL: orderbook depth ticks; level field is ignored and not stored;
 *          use when per-level identity is irrelevant (e.g., mid-price,
 *          spread calculations) or to store a single level such as BBO.
 */
enum class MarketDataMode : uint8_t {
    ALL       = 0, ///< All fields stored (timestamp, side, level, price, quantity, orders).
    TRADE     = 1, ///< Trade ticks only; level field ignored and not stored.
    LIQUIDITY = 2, ///< Orderbook liquidity updates; all fields stored.
    LEVEL     = 3  ///< Depth ticks only; level field ignored and not stored.
};

/**
 * @brief Convert a MarketDataMode value to its lowercase string representation.
 * @param mode The MarketDataMode enum value to convert.
 * @return One of: "all", "trade", "liquidity", "level".
 */
[[gnu::always_inline]] inline std::string_view to_string(const MarketDataMode mode) noexcept
{
    static constexpr std::string_view table[] = {"all", "trade", "liquidity", "level"};
    return table[static_cast<uint8_t>(mode)];
}

} // namespace marketkernel
