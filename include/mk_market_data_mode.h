/* Market Kernel : MarketDataMode Enum
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace marketkernel {

/*
 * Controls which fields are stored per tick.
 *
 * ALL       - stores every field: timestamp, side, level, price, quantity, orders.
 *             Use for full orderbook replay or multi-purpose datasets.
 *
 * TRADE     - trade ticks only; level field is ignored and not stored.
 *             Use when only last-traded-price/quantity data is needed.
 *
 * LIQUIDITY - stores timestamp, side, level, price, quantity, orders.
 *             Same fields as ALL but semantically scoped to orderbook
 *             liquidity updates (adds, modifies, deletes).
 *
 * LEVEL     - orderbook depth ticks only; level field is ignored and not stored.
 *             Use when per-level identity is irrelevant and only price/quantity
 *             at each side matters (e.g., mid-price, spread calculations), or
 *             to store a single level such as top-of-book (BBO).
 */
enum class MarketDataMode : uint8_t {
    ALL       = 0,
    TRADE     = 1,
    LIQUIDITY = 2,
    LEVEL     = 3
};

[[gnu::always_inline]] inline std::string_view to_string(const MarketDataMode mode) noexcept
{
    static constexpr std::string_view table[] = {"all", "trade", "liquidity", "level"};
    return table[static_cast<uint8_t>(mode)];
}

} // namespace marketkernel
