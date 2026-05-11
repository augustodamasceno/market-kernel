

# File mk\_side.h

[**File List**](files.md) **>** [**include**](dir_d44c64559bbebec7f509842c48db8b23.md) **>** [**mk\_side.h**](mk__side_8h.md)

[Go to the documentation of this file](mk__side_8h.md)


```C++
/* Market Kernel : Side Enum
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */


#pragma once

#include <cstdint>

namespace marketkernel {

enum class Side : uint8_t {
    BUY  = 0, 
    SELL = 1  
};

[[gnu::always_inline]] inline std::string_view to_string(const Side side) noexcept
{
    return (static_cast<uint8_t>(side) & 1U) ? "sell" : "buy";
}


} // namespace marketkernel
```


