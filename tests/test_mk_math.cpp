/* Market Kernel : Unit Tests for Utilities
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "mk_math.hpp"

namespace {

using marketkernel::simd_sum;

TEST(MathUtilsTest, SimdSumGenericIntUsesAccumulateSemantics)
{
    const std::vector<int> data {1, 2, 3, -4, 10, -2};
    EXPECT_EQ(simd_sum<int>(data), 10);
    EXPECT_EQ(simd_sum<int>({}), 0);
}

TEST(MathUtilsTest, SimdSumFloatHandlesEmptyAndSingleValue)
{
    EXPECT_FLOAT_EQ(simd_sum<float>({}), 0.0F);
    EXPECT_FLOAT_EQ(simd_sum<float>({3.5F}), 3.5F);
}

TEST(MathUtilsTest, SimdSumFloatMatchesAccumulateOnTailSensitiveSize)
{
    std::vector<float> data(37U);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<float>(i + 1U);
    }

    const float expected = std::accumulate(data.begin(), data.end(), 0.0F);
    EXPECT_FLOAT_EQ(simd_sum<float>(data), expected);
}

TEST(MathUtilsTest, SimdSumDoubleHandlesEmptyAndSingleValue)
{
    EXPECT_DOUBLE_EQ(simd_sum<double>({}), 0.0);
    EXPECT_DOUBLE_EQ(simd_sum<double>({2.25}), 2.25);
}

TEST(MathUtilsTest, SimdSumDoubleMatchesAccumulateOnTailSensitiveSize)
{
    std::vector<double> data(23U);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<double>(i) * 0.5;
    }

    const double expected = std::accumulate(data.begin(), data.end(), 0.0);
    EXPECT_DOUBLE_EQ(simd_sum<double>(data), expected);
}

} // namespace
