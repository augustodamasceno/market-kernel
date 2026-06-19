/* Market Kernel : Side Enum
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_side.h
 * @brief Side enum and string conversion helper.
 */

#pragma once

#include <cstdint>

namespace marketkernel {

/**
 * @brief Order side for a market tick: buy or sell.
 */
enum class Side : uint8_t {
    BUY  = 0, ///< Buy (bid) side.
    SELL = 1  ///< Sell (ask) side.
};

/**
 * @brief Convert a Side value to its lowercase string representation.
 *
 * @details Uses a branchless static lookup table indexed by the underlying
 * integer value of the enum, avoiding conditional branch mispredictions.
 *
 * @param[in] side The Side enum value to convert.
 * @return "buy" for Side::BUY, "sell" for Side::SELL.
 *
 * @note Time Complexity: $O(1)$
 * @note Space Complexity: $O(1)$
 * @note Memory: Zero heap allocations.
 */
[[gnu::always_inline]] inline std::string_view to_string(const Side side) noexcept
{
    static constexpr std::string_view table[] = { "buy", "sell" };
    return table[static_cast<uint8_t>(side)];
}


} // namespace marketkernel
