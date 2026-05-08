/* Market Kernel : Unit Tests for MarketData
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "mk_market_data.hpp"

namespace {

using marketkernel::MarketData;
using marketkernel::MarketDataMode;
using marketkernel::Side;

TEST(MarketDataTest, DefaultConstructedObjectIsEmpty)
{
    const MarketData<float> md;

    EXPECT_TRUE(md.symbol().empty());
    EXPECT_EQ(md.mode(), MarketDataMode::ALL);
    EXPECT_TRUE(md.empty());
    EXPECT_EQ(md.size(), 0U);
    EXPECT_TRUE(md.timestamps().empty());
    EXPECT_TRUE(md.sides().empty());
    EXPECT_TRUE(md.levels().empty());
    EXPECT_TRUE(md.prices().empty());
    EXPECT_TRUE(md.quantities().empty());
    EXPECT_TRUE(md.orders().empty());
}

TEST(MarketDataTest, AppendInAllModeStoresAllFields)
{
    MarketData<double> md("es", MarketDataMode::ALL);

    md.append(1001ULL, Side::BUY, 2U, 5300.25, 5.0, 3.0);
    md.append(1002ULL, Side::SELL, 1U, 5300.00, 2.0, 1.0);

    ASSERT_EQ(md.size(), 2U);
    ASSERT_EQ(md.timestamps().size(), 2U);
    ASSERT_EQ(md.sides().size(), 2U);
    ASSERT_EQ(md.levels().size(), 2U);
    ASSERT_EQ(md.prices().size(), 2U);
    ASSERT_EQ(md.quantities().size(), 2U);
    ASSERT_EQ(md.orders().size(), 2U);

    EXPECT_EQ(md.timestamps()[0], 1001ULL);
    EXPECT_EQ(md.sides()[0], Side::BUY);
    EXPECT_EQ(md.levels()[0], 2U);
    EXPECT_DOUBLE_EQ(md.prices()[0], 5300.25);
    EXPECT_DOUBLE_EQ(md.quantities()[0], 5.0);
    EXPECT_DOUBLE_EQ(md.orders()[0], 3.0);

    EXPECT_EQ(md.timestamps()[1], 1002ULL);
    EXPECT_EQ(md.sides()[1], Side::SELL);
    EXPECT_EQ(md.levels()[1], 1U);
}

TEST(MarketDataTest, TradeModeDoesNotStoreLevels)
{
    MarketData<float> md("nvda", MarketDataMode::TRADE);

    md.append(11ULL, Side::BUY, 0U, 100.5F, 10.0F, 1.0F);
    md.append(12ULL, Side::SELL, 0U, 101.0F, 8.0F, 2.0F);

    EXPECT_EQ(md.size(), 2U);
    EXPECT_TRUE(md.levels().empty());
    EXPECT_EQ(md.timestamps().size(), 2U);
    EXPECT_EQ(md.sides().size(), 2U);
    EXPECT_EQ(md.prices().size(), 2U);
}

TEST(MarketDataTest, TradeModeDropsNonZeroLevels)
{
    // TRADE mode sets max_level_=0 at construction, so any tick with level > 0
    // is dropped by the normal max_level check in append().
    MarketData<float> md("nvda", MarketDataMode::TRADE);

    md.append(1ULL, Side::BUY, 0U, 50.0F, 1.0F, 1.0F);  // level 0: stored
    md.append(2ULL, Side::BUY, 1U, 60.0F, 1.0F, 1.0F);  // level 1 > 0: dropped

    EXPECT_EQ(md.size(), 1U);
    EXPECT_FLOAT_EQ(md.prices()[0], 50.0F);
}

TEST(MarketDataTest, LiquidityModeStoresLevels)
{
    MarketData<float> md("asml", MarketDataMode::LIQUIDITY);

    md.append(21ULL, Side::BUY, 4U, 900.0F, 20.0F, 6.0F);

    ASSERT_EQ(md.size(), 1U);
    ASSERT_EQ(md.levels().size(), 1U);
    EXPECT_EQ(md.levels()[0], 4U);
}

TEST(MarketDataTest, LevelModeDoesNotStoreLevels)
{
    MarketData<float> md("es", MarketDataMode::LEVEL);

    md.append(31ULL, Side::SELL, 2U, 4200.0F, 1.0F, 1.0F);

    EXPECT_EQ(md.size(), 1U);
    EXPECT_TRUE(md.levels().empty());
}

TEST(MarketDataTest, ClearRemovesTicksButKeepsSymbol)
{
    MarketData<float> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY, 1U, 10.0F, 2.0F, 1.0F);
    md.append(2ULL, Side::SELL, 2U, 11.0F, 3.0F, 2.0F);

    md.clear();

    EXPECT_EQ(md.symbol(), "es");
    EXPECT_TRUE(md.empty());
    EXPECT_EQ(md.size(), 0U);
    EXPECT_TRUE(md.timestamps().empty());
    EXPECT_TRUE(md.sides().empty());
    EXPECT_TRUE(md.levels().empty());
    EXPECT_TRUE(md.prices().empty());
    EXPECT_TRUE(md.quantities().empty());
    EXPECT_TRUE(md.orders().empty());
}

TEST(MarketDataTest, ToStringEmptyDataReturnsHeaderOnly)
{
    MarketData<float> md("es", MarketDataMode::ALL);

    const std::string s = md.to_string();
    EXPECT_EQ(s, "symbol,mode,timestamp,side,level,price,quantity,orders\n");
}

TEST(MarketDataTest, ToStringAllModeRow)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1001ULL, Side::BUY, 2U, 5300.125, 10.5, 3.0);

    const std::string s = md.to_string();
    EXPECT_NE(s.find("symbol,mode,timestamp,side,level,price,quantity,orders\n"),
              std::string::npos);
    EXPECT_NE(s.find("es,all,1001,buy,2,5300.125000,10.500000,3.000000\n"),
              std::string::npos);
}

TEST(MarketDataTest, ToStringTradeModeEmptyLevelField)
{
    MarketData<float> md("nvda", MarketDataMode::TRADE);
    md.append(2002ULL, Side::SELL, 0U, 100.25F, 4.0F, 1.0F);

    const std::string s = md.to_string();
    // level column header still present (uniform schema)
    EXPECT_NE(s.find("symbol,mode,timestamp,side,level,price,quantity,orders\n"),
              std::string::npos);
    // level field is empty between two commas
    EXPECT_NE(s.find("nvda,trade,2002,sell,,100.250000,4.000000,1.000000\n"),
              std::string::npos);
}

TEST(MarketDataTest, ToStringNoHeader)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY, 1U, 10.0, 1.0, 1.0);

    const std::string s = md.to_string(false);
    EXPECT_EQ(s.find("symbol,mode"), std::string::npos);
    EXPECT_NE(s.find("es,all,1,buy,1,"), std::string::npos);
}

TEST(MarketDataTest, ToStringCustomDecimalPlaces)
{
    MarketData<double> md("asml", MarketDataMode::LIQUIDITY);
    md.append(3003ULL, Side::BUY, 1U, 123.456789, 9.876543, 2.5);

    const std::string s = md.to_string(true, 3);
    EXPECT_NE(s.find("asml,liquidity,3003,buy,1,123.457,9.877,2.500\n"),
              std::string::npos);
}

TEST(MarketDataTest, ToStringHalfOpenRowRange)
{
    MarketData<float> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0F, 1.0F, 1.0F);
    md.append(2ULL, Side::SELL, 2U, 20.0F, 2.0F, 2.0F);
    md.append(3ULL, Side::BUY,  3U, 30.0F, 3.0F, 3.0F);

    // rows [1, 2) → only row index 1
    const std::string s = md.to_string(true, 6, 1U, 2U);
    EXPECT_NE(s.find("es,all,2,sell,2,"), std::string::npos);
    EXPECT_EQ(s.find("es,all,1,buy,"),    std::string::npos);
    EXPECT_EQ(s.find("es,all,3,buy,"),    std::string::npos);
}

TEST(MarketDataTest, ToStringInvalidRangeReturnsEmpty)
{
    MarketData<float> md("nvda", MarketDataMode::TRADE);
    md.append(1ULL, Side::BUY, 0U, 1.0F, 1.0F, 1.0F);

    // start > end
    EXPECT_TRUE(md.to_string(true, 6, 2U, 1U).empty());
    // end_exclusive > size
    EXPECT_TRUE(md.to_string(true, 6, 0U, 2U).empty());
}

TEST(MarketDataTest, ToStringMultipleRows)
{
    MarketData<double> md("aapl", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 1.0, 1.0, 1.0);
    md.append(2ULL, Side::SELL, 2U, 2.0, 2.0, 2.0);

    const std::string s = md.to_string();
    EXPECT_NE(s.find("aapl,all,1,buy,1,"),  std::string::npos);
    EXPECT_NE(s.find("aapl,all,2,sell,2,"), std::string::npos);
}

TEST(MarketDataTest, ToCsvReturnsFalseOnEmpty)
{
    MarketData<float> md("es", MarketDataMode::ALL);
    EXPECT_FALSE(md.to_csv("out_empty.csv", 100));
}

TEST(MarketDataTest, ToCsvReturnsFalseOnZeroChunk)
{
    MarketData<float> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY, 1U, 1.0F, 1.0F, 1.0F);
    EXPECT_FALSE(md.to_csv("out_zero_chunk.csv", 0));
}

TEST(MarketDataTest, ToCsvSingleChunk)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0, 1.0, 1.0);
    md.append(2ULL, Side::SELL, 2U, 20.0, 2.0, 2.0);

    const std::string path = "test_market_data_ToCsvSingleChunk.csv";
    ASSERT_TRUE(md.to_csv(path, 100));

    std::string content;
    {
        std::ifstream f(path);
        ASSERT_TRUE(f.is_open());
        content.assign((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    }
    std::remove(path.c_str());

    EXPECT_NE(content.find("symbol,mode,timestamp,side,level,price,quantity,orders\n"),
              std::string::npos);
    EXPECT_NE(content.find("es,all,1,buy,1,"),  std::string::npos);
    EXPECT_NE(content.find("es,all,2,sell,2,"), std::string::npos);
    // header appears exactly once
    const std::size_t first = content.find("symbol,mode");
    EXPECT_EQ(content.find("symbol,mode", first + 1), std::string::npos);
}

TEST(MarketDataTest, ToCsvMultipleChunks)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0, 1.0, 1.0);
    md.append(2ULL, Side::SELL, 2U, 20.0, 2.0, 2.0);
    md.append(3ULL, Side::BUY,  3U, 30.0, 3.0, 3.0);

    const std::string path = "test_market_data_ToCsvMultipleChunks.csv";
    ASSERT_TRUE(md.to_csv(path, 1));

    std::string content;
    {
        std::ifstream f(path);
        ASSERT_TRUE(f.is_open());
        content.assign((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    }
    std::remove(path.c_str());

    // all rows present
    EXPECT_NE(content.find("es,all,1,buy,1,"),  std::string::npos);
    EXPECT_NE(content.find("es,all,2,sell,2,"), std::string::npos);
    EXPECT_NE(content.find("es,all,3,buy,3,"),  std::string::npos);
    // header appears exactly once
    const std::size_t first = content.find("symbol,mode");
    EXPECT_EQ(content.find("symbol,mode", first + 1), std::string::npos);
}

TEST(MarketDataTest, ToCsvOverwritesExistingFile)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY, 1U, 1.0, 1.0, 1.0);

    const std::string path = "test_to_csv_overwrite.csv";
    ASSERT_TRUE(md.to_csv(path, 100));
    ASSERT_TRUE(md.to_csv(path, 100));

    std::string content;
    {
        std::ifstream f(path);
        content.assign((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    }
    std::remove(path.c_str());

    // header appears exactly once (file was truncated, not appended)
    const std::size_t first = content.find("symbol,mode");
    EXPECT_EQ(content.find("symbol,mode", first + 1), std::string::npos);
}

TEST(MarketDataTest, DefaultMaxLevelIs4)
{
    const MarketData<float> md;
    EXPECT_EQ(md.max_level(), 4U);
}

TEST(MarketDataTest, AppendIgnoredWhenLevelExceedsMaxLevel)
{
    MarketData<float> md("es", MarketDataMode::ALL);

    md.append(1ULL, Side::BUY, 4U, 10.0F, 1.0F, 1.0F);  // level == max_level: stored
    md.append(2ULL, Side::BUY, 5U, 20.0F, 2.0F, 2.0F);  // level > max_level: ignored

    EXPECT_EQ(md.size(), 1U);
    EXPECT_FLOAT_EQ(md.prices()[0], 10.0F);
}

TEST(MarketDataTest, CustomMaxLevelConstructorFiltersCorrectly)
{
    MarketData<double> md("nvda", MarketDataMode::ALL, 2U);

    EXPECT_EQ(md.max_level(), 2U);
    md.append(1ULL, Side::BUY,  1U, 100.0, 1.0, 1.0);  // stored
    md.append(2ULL, Side::SELL, 2U, 101.0, 1.0, 1.0);  // stored
    md.append(3ULL, Side::BUY,  3U, 102.0, 1.0, 1.0);  // ignored

    EXPECT_EQ(md.size(), 2U);
}

TEST(MarketDataTest, OperatorOstreamOutputsFullObject)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0, 1.0, 1.0);
    md.append(2ULL, Side::SELL, 2U, 20.0, 2.0, 2.0);

    std::ostringstream oss;
    oss << md;

    EXPECT_EQ(oss.str(), md.to_string());
}

TEST(MarketDataTest, OperatorOstreamLimitedRowsViaToString)
{
    MarketData<float> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0F, 1.0F, 1.0F);
    md.append(2ULL, Side::SELL, 2U, 20.0F, 2.0F, 2.0F);
    md.append(3ULL, Side::BUY,  3U, 30.0F, 3.0F, 3.0F);

    std::ostringstream oss;
    oss << md;
    const std::string full    = oss.str();
    const std::string partial = md.to_string(true, 6, 1U, 2U);

    // operator<< outputs all three rows
    EXPECT_NE(full.find("es,all,1,buy,1,"),    std::string::npos);
    EXPECT_NE(full.find("es,all,2,sell,2,"),   std::string::npos);
    EXPECT_NE(full.find("es,all,3,buy,3,"),    std::string::npos);
    // to_string with range [1,2) contains only row index 1
    EXPECT_EQ(partial.find("es,all,1,buy,1,"),  std::string::npos);
    EXPECT_NE(partial.find("es,all,2,sell,2,"), std::string::npos);
    EXPECT_EQ(partial.find("es,all,3,buy,3,"),  std::string::npos);
}

TEST(MarketDataTest, OperatorIstreamAppendsOneRow)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY, 2U, 100.0, 5.0, 3.0);
    const std::string row = md.to_string(false);  // data row, no header
    md.clear();

    std::istringstream ss(row);
    ss >> md;

    ASSERT_FALSE(ss.fail());
    ASSERT_EQ(md.size(), 1U);
    EXPECT_EQ(md.timestamps()[0], 1ULL);
    EXPECT_EQ(md.sides()[0], Side::BUY);
    EXPECT_EQ(md.levels()[0], 2U);
    EXPECT_DOUBLE_EQ(md.prices()[0], 100.0);
    EXPECT_DOUBLE_EQ(md.quantities()[0], 5.0);
    EXPECT_DOUBLE_EQ(md.orders()[0], 3.0);
}

TEST(MarketDataTest, OperatorIstreamSetsFailbitOnBadRow)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    std::istringstream ss("NOT A VALID CSV ROW\n");
    ss >> md;

    EXPECT_TRUE(ss.fail());
    EXPECT_EQ(md.size(), 0U);
}

TEST(MarketDataTest, LoadFromCsvFile)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    md.append(1ULL, Side::BUY,  1U, 10.0, 1.0, 1.0);
    md.append(2ULL, Side::SELL, 2U, 20.0, 2.0, 2.0);
    const std::string path = "test_load_from_csv.csv";
    ASSERT_TRUE(md.to_csv(path, 100));

    MarketData<double> md2("es", MarketDataMode::ALL);
    ASSERT_TRUE(md2.load(path));
    std::remove(path.c_str());

    ASSERT_EQ(md2.size(), 2U);
    EXPECT_EQ(md2.timestamps()[0], 1ULL);
    EXPECT_DOUBLE_EQ(md2.prices()[0], 10.0);
    EXPECT_EQ(md2.timestamps()[1], 2ULL);
    EXPECT_DOUBLE_EQ(md2.prices()[1], 20.0);
}

TEST(MarketDataTest, LoadReturnsFalseOnMissingFile)
{
    MarketData<double> md("es", MarketDataMode::ALL);
    EXPECT_FALSE(md.load("no_such_file_xyz.csv"));
}

TEST(MarketDataTest, PeekCsvReturnsSymbolAndMode)
{
    MarketData<double> md("nvda", MarketDataMode::LIQUIDITY);
    md.append(1ULL, Side::BUY, 1U, 100.0, 1.0, 1.0);
    const std::string path = "test_peek_csv.csv";
    ASSERT_TRUE(md.to_csv(path, 100));

    const auto result = MarketData<double>::peek_csv(path);
    std::remove(path.c_str());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first,  "nvda");
    EXPECT_EQ(result->second, MarketDataMode::LIQUIDITY);
}

TEST(MarketDataTest, PeekCsvReturnsNulloptOnMissingFile)
{
    EXPECT_FALSE(MarketData<double>::peek_csv("no_such_file_xyz.csv").has_value());
}

TEST(MarketDataTest, PeekCsvReturnsNulloptOnHeaderOnlyFile)
{
    const std::string path = "test_peek_csv_header_only.csv";
    {
        std::ofstream f(path);
        f << "symbol,mode,timestamp,side,level,price,quantity,orders\n";
    }
    EXPECT_FALSE(MarketData<double>::peek_csv(path).has_value());
    std::remove(path.c_str());
}

TEST(MarketDataTest, LoadSkipsBadLinesAndContinues)
{
    const std::string path = "test_load_bad_lines.csv";
    {
        std::ofstream f(path);
        f << "symbol,mode,timestamp,side,level,price,quantity,orders\n";
        f << "es,all,1,buy,1,10.000000,1.000000,1.000000\n";
        f << "THIS IS A BAD LINE\n";
        f << "es,all,2,sell,2,20.000000,2.000000,2.000000\n";
    }

    MarketData<double> md("es", MarketDataMode::ALL);
    ASSERT_TRUE(md.load(path));
    std::remove(path.c_str());

    ASSERT_EQ(md.size(), 2U);
    EXPECT_EQ(md.timestamps()[0], 1ULL);
    EXPECT_EQ(md.timestamps()[1], 2ULL);
}

} // namespace
