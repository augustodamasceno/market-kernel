

# File mk\_market\_data\_mode.h

[**File List**](files.md) **>** [**include**](dir_d44c64559bbebec7f509842c48db8b23.md) **>** [**mk\_market\_data\_mode.h**](mk__market__data__mode_8h.md)

[Go to the documentation of this file](mk__market__data__mode_8h.md)


```C++
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
```


