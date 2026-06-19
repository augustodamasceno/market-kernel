/* Market Kernel : Unit Tests for MarketDataView
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file test_mk_market_data_view.cpp
 * @brief Unit tests for the MarketDataView class and the Span helper.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <string>

#include "mk_market_data.hpp"
#include "mk_market_data_view.hpp"

namespace {

using marketkernel::MarketData;
using marketkernel::MarketDataMode;
using marketkernel::MarketDataView;
using marketkernel::Side;
using marketkernel::Span;

static std::string temp_path(const std::string& name)
{
    return "/tmp/mk_view_test_" + name;
}

/// @brief Build a standard 3-tick double ALL-mode file and save it to @p path.
static bool make_all_mode_file(const std::string& path)
{
    MarketData<double> md("es", MarketDataMode::ALL, 4U);
    md.append(1000ULL, Side::BUY,  1U, 5300.25, 10.0, 3.0);
    md.append(1001ULL, Side::SELL, 2U, 5299.00,  5.0, 1.0);
    md.append(1002ULL, Side::BUY,  0U, 5300.50,  2.0, 1.0);
    return md.save_binary(path);
}

TEST(MarketDataViewTest, DefaultConstructedIsInvalid)
{
    const MarketDataView<double> view;

    EXPECT_FALSE(view.valid());
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0U);
    EXPECT_TRUE(view.symbol().empty());
    EXPECT_TRUE(view.timestamps().empty());
    EXPECT_TRUE(view.sides().empty());
    EXPECT_TRUE(view.levels().empty());
    EXPECT_TRUE(view.prices().empty());
    EXPECT_TRUE(view.quantities().empty());
    EXPECT_TRUE(view.orders().empty());
}

TEST(MarketDataViewTest, NonExistentPathIsInvalid)
{
    const MarketDataView<double> view("/nonexistent/path/mk_view.mkmd");
    EXPECT_FALSE(view.valid());
}

TEST(MarketDataViewTest, ValidFileIsValid)
{
    const std::string path = temp_path("valid.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    EXPECT_TRUE(view.valid());

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, MetadataMatchesSavedFile)
{
    const std::string path = temp_path("metadata.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    EXPECT_EQ(view.symbol(),    "es");
    EXPECT_EQ(view.mode(),      MarketDataMode::ALL);
    EXPECT_EQ(view.max_level(), 4U);
    EXPECT_EQ(view.size(),      3U);
    EXPECT_FALSE(view.empty());

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, TimestampsMatchSavedData)
{
    const std::string path = temp_path("ts.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_EQ(view.timestamps().size(), 3U);
    EXPECT_EQ(view.timestamps()[0], 1000ULL);
    EXPECT_EQ(view.timestamps()[1], 1001ULL);
    EXPECT_EQ(view.timestamps()[2], 1002ULL);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, SidesMatchSavedData)
{
    const std::string path = temp_path("sides.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_EQ(view.sides().size(), 3U);
    EXPECT_EQ(view.sides()[0], Side::BUY);
    EXPECT_EQ(view.sides()[1], Side::SELL);
    EXPECT_EQ(view.sides()[2], Side::BUY);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, LevelsMatchSavedDataInAllMode)
{
    const std::string path = temp_path("levels.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_FALSE(view.levels().empty());
    ASSERT_EQ(view.levels().size(), 3U);
    EXPECT_EQ(view.levels()[0], 1U);
    EXPECT_EQ(view.levels()[1], 2U);
    EXPECT_EQ(view.levels()[2], 0U);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, PricesMatchSavedData)
{
    const std::string path = temp_path("prices.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_EQ(view.prices().size(), 3U);
    EXPECT_DOUBLE_EQ(view.prices()[0], 5300.25);
    EXPECT_DOUBLE_EQ(view.prices()[1], 5299.00);
    EXPECT_DOUBLE_EQ(view.prices()[2], 5300.50);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, QuantitiesMatchSavedData)
{
    const std::string path = temp_path("qty.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_EQ(view.quantities().size(), 3U);
    EXPECT_DOUBLE_EQ(view.quantities()[0], 10.0);
    EXPECT_DOUBLE_EQ(view.quantities()[1],  5.0);
    EXPECT_DOUBLE_EQ(view.quantities()[2],  2.0);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, OrdersMatchSavedData)
{
    const std::string path = temp_path("orders.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    ASSERT_EQ(view.orders().size(), 3U);
    EXPECT_DOUBLE_EQ(view.orders()[0], 3.0);
    EXPECT_DOUBLE_EQ(view.orders()[1], 1.0);
    EXPECT_DOUBLE_EQ(view.orders()[2], 1.0);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, TradeModeHasEmptyLevelsSpan)
{
    const std::string path = temp_path("trade_mode.mkmd");
    MarketData<double> md("nvda", MarketDataMode::TRADE);
    md.append(2000ULL, Side::BUY, 0U, 100.0, 1.0, 1.0);
    md.append(2001ULL, Side::SELL, 0U, 99.5, 2.0, 1.0);
    ASSERT_TRUE(md.save_binary(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    EXPECT_EQ(view.mode(), MarketDataMode::TRADE);
    EXPECT_EQ(view.size(), 2U);
    EXPECT_TRUE(view.levels().empty());
    EXPECT_EQ(view.levels().size(), 0U);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, LevelModeHasEmptyLevelsSpan)
{
    const std::string path = temp_path("level_mode.mkmd");
    MarketData<double> md("asml", MarketDataMode::LEVEL);
    md.append(3000ULL, Side::BUY, 0U, 200.0, 3.0, 1.0);
    ASSERT_TRUE(md.save_binary(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    EXPECT_EQ(view.mode(), MarketDataMode::LEVEL);
    EXPECT_TRUE(view.levels().empty());

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, FloatViewLoadsCorrectly)
{
    const std::string path = temp_path("float.mkmd");
    MarketData<float> md("asml", MarketDataMode::ALL, 2U);
    md.append(3000ULL, Side::BUY,  1U, 900.0F, 5.0F, 2.0F);
    md.append(3001ULL, Side::SELL, 0U, 899.5F, 3.0F, 1.0F);
    ASSERT_TRUE(md.save_binary(path));

    const MarketDataView<float> view(path);
    ASSERT_TRUE(view.valid());

    EXPECT_EQ(view.symbol(),    "asml");
    EXPECT_EQ(view.max_level(), 2U);
    ASSERT_EQ(view.size(),      2U);
    EXPECT_FLOAT_EQ(view.prices()[0], 900.0F);
    EXPECT_FLOAT_EQ(view.prices()[1], 899.5F);
    EXPECT_FLOAT_EQ(view.quantities()[0], 5.0F);
    EXPECT_FLOAT_EQ(view.orders()[1], 1.0F);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, NumTypeMismatchIsRejected)
{
    const std::string path = temp_path("type_mismatch.mkmd");
    MarketData<double> md("es");
    md.append(1ULL, Side::BUY, 1U, 100.0, 1.0, 1.0);
    ASSERT_TRUE(md.save_binary(path));

    // A double file opened as float must be rejected.
    const MarketDataView<float> bad_view(path);
    EXPECT_FALSE(bad_view.valid());

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, SpanIsRangeIterable)
{
    const std::string path = temp_path("range_iter.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    double sum = 0.0;
    for (const double p : view.prices())
        sum += p;

    EXPECT_DOUBLE_EQ(sum, 5300.25 + 5299.00 + 5300.50);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, SpanDataAndSizeMethods)
{
    const std::string path = temp_path("span_methods.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    const Span<double> qty = view.quantities();
    EXPECT_EQ(qty.size(), 3U);
    EXPECT_NE(qty.data(), nullptr);
    EXPECT_DOUBLE_EQ(qty.data()[0], 10.0);
    EXPECT_DOUBLE_EQ(qty.data()[1],  5.0);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, EmptySpanProperties)
{
    const Span<double> empty_span{nullptr, 0U};
    EXPECT_TRUE(empty_span.empty());
    EXPECT_EQ(empty_span.size(), 0U);
    EXPECT_EQ(empty_span.data(), nullptr);
    EXPECT_EQ(empty_span.begin(), empty_span.end());
}

TEST(MarketDataViewTest, MoveConstructorTransfersOwnership)
{
    const std::string path = temp_path("move_ctor.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    MarketDataView<double> view1(path);
    ASSERT_TRUE(view1.valid());

    MarketDataView<double> view2(std::move(view1));

    EXPECT_FALSE(view1.valid());   // NOLINT: intentional use after move for state check
    EXPECT_TRUE(view2.valid());
    EXPECT_EQ(view2.symbol(), "es");
    EXPECT_EQ(view2.size(), 3U);
    EXPECT_DOUBLE_EQ(view2.prices()[0], 5300.25);

    std::remove(path.c_str());
}

TEST(MarketDataViewTest, MoveAssignmentTransfersOwnership)
{
    const std::string path = temp_path("move_assign.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    MarketDataView<double> view1(path);
    ASSERT_TRUE(view1.valid());

    MarketDataView<double> view2;
    view2 = std::move(view1);

    EXPECT_FALSE(view1.valid());   // NOLINT: intentional use after move for state check
    EXPECT_TRUE(view2.valid());
    EXPECT_EQ(view2.symbol(), "es");
    EXPECT_EQ(view2.size(), 3U);

    std::remove(path.c_str());
}

/// @brief A template algorithm that sums prices over any MarketData-compatible type.
template<typename Source>
static double sum_prices(const Source& src)
{
    double total = 0.0;
    for (const auto& p : src.prices())
        total += static_cast<double>(p);
    return total;
}

TEST(MarketDataViewTest, TemplateAlgorithmWorksWithBothTypes)
{
    const std::string path = temp_path("template_compat.mkmd");
    ASSERT_TRUE(make_all_mode_file(path));

    MarketData<double> md("es", MarketDataMode::ALL, 4U);
    md.append(1000ULL, Side::BUY,  1U, 5300.25, 10.0, 3.0);
    md.append(1001ULL, Side::SELL, 2U, 5299.00,  5.0, 1.0);
    md.append(1002ULL, Side::BUY,  0U, 5300.50,  2.0, 1.0);

    const MarketDataView<double> view(path);
    ASSERT_TRUE(view.valid());

    const double expected = 5300.25 + 5299.00 + 5300.50;
    EXPECT_DOUBLE_EQ(sum_prices(md),   expected);
    EXPECT_DOUBLE_EQ(sum_prices(view), expected);

    std::remove(path.c_str());
}

} // namespace
