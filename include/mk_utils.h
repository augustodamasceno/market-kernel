/* Market Kernel : Utilities
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_utils.h
 * @brief Utility constants and helpers for the Market Kernel library.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace marketkernel {

/**
 * @brief Compile-time flag indicating the host byte order.
 *
 * @details Evaluated entirely at compile time using well-known predefined
 * macros.  No runtime probe or branch is generated.
 *
 * - GCC / Clang / ICC: uses @c __BYTE_ORDER__ and @c __ORDER_LITTLE_ENDIAN__.
 * - MSVC: assumed little-endian; MSVC only targets x86/x86-64/ARM (all LE).
 * - Any other compiler that cannot supply the macros: triggers a hard
 *   @c #error so a mis-detected endianness is never silently accepted.
 *
 * @note @c true  = little-endian (LSB at lowest address).
 * @note @c false = big-endian    (MSB at lowest address).
 * @warning Endianness resolved at compile time from the BUILD machine.
 *          If running this binary on a different architecture,
 *          call check_endianness() at startup.
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
static constexpr bool IS_LITTLE_ENDIAN = (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#elif defined(_MSC_VER)
static constexpr bool IS_LITTLE_ENDIAN = true; // MSVC only targets LE
#else
#error "Cannot determine endianness at compile time"
#endif

/**
 * @brief Round @p value up to the nearest multiple of @p alignment.
 *
 * @details Uses a single bitwise-AND mask; no division or branch is generated.
 *
 * @pre @p alignment must be a non-zero power of two.
 *
 * @param[in] value     The value to align.
 * @param[in] alignment The required alignment boundary (must be a power of two).
 * @return The smallest multiple of @p alignment that is >= @p value.
 *
 * @note Time Complexity: $O(1)$
 * @note Space Complexity: $O(1)$
 * @note Memory: Zero heap allocations.
 */
[[gnu::always_inline]] inline constexpr std::size_t align_up(
    std::size_t value, std::size_t alignment) noexcept
{
    return (value + alignment - 1U) & ~(alignment - 1U);
}

/**
 *        assumption encoded in IS_LITTLE_ENDIAN.
 *
 * @details Performs a one-byte memory probe using @c std::memcpy to avoid
 * strict-aliasing undefined behaviour.  Should be called once at program
 * startup — typically in @c main() before any binary I/O — whenever the
 * binary might be deployed on a machine with a different architecture than
 * the build machine.
 *
 * Terminates the process via @c std::abort() with a diagnostic message to
 * @c stderr if a mismatch is detected.
 *
 * @pre None.
 * @post Process continues normally if endianness matches; aborts otherwise.
 *
 * @note Time Complexity: $O(1)$
 * @note Space Complexity: $O(1)$
 * @note Memory: Zero heap allocations.
 */
[[gnu::always_inline]] inline void check_endianness() noexcept
{
    uint16_t s = 0x0001U;
    uint8_t  b;
    std::memcpy(&b, &s, 1);
    const bool runtime_le = (b == 1);

    if (runtime_le != IS_LITTLE_ENDIAN)
    {
        std::fprintf(stderr,
            "[FATAL] Endianness mismatch: compiled for %s but running on %s\n",
            IS_LITTLE_ENDIAN ? "little-endian" : "big-endian",
            runtime_le       ? "little-endian" : "big-endian");
        std::abort();
    }
}

} // namespace marketkernel
