/* Market Kernel : MarketData Class
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "mk_market_data_mode.h"
#include "mk_side.h"

namespace marketkernel {

/*
 * Structure-of-Arrays (SoA) layout for low-latency bulk calculations.
 *
 * Each tick field is stored in its own contiguous vector so that
 * operations on a single field (e.g., iterating all prices) load only
 * the relevant data into cache and are SIMD-friendly.
 *
 * level == 0  -> trade (last traded price/quantity)
 * level >= 1  -> orderbook depth (1 = top of book, ...)
 *
 * Call reserve before the session starts to avoid reallocations.
 *
 * Mode controls which fields are stored per append call:
 *   ALL       - all fields stored, including level. Default.
 *   TRADE     - level ignored and not stored; suited for trade-only feeds.
 *   LIQUIDITY - all fields stored, including level; semantically scoped to
 *               orderbook liquidity updates.
 *   LEVEL     - level ignored and not stored; suited for depth snapshots where
 *               per-level identity is not needed, or to store a single level
 *               such as top-of-book (BBO).
 *
 * max_level controls the maximum accepted level value. append() is a no-op
 * when level > max_level. Default is 4.
 */
template<typename Num>
class alignas(64) MarketData
{
public:
    MarketData() = default;
    explicit MarketData(const std::string& symbol);
    MarketData(const std::string& symbol, MarketDataMode mode);
    MarketData(const std::string& symbol, MarketDataMode mode, uint8_t max_level);

    [[gnu::always_inline]] inline std::string_view symbol()    const noexcept;
    [[gnu::always_inline]] inline MarketDataMode   mode()      const noexcept;
    [[gnu::always_inline]] inline uint8_t          max_level() const noexcept;
    [[gnu::always_inline]] inline std::size_t      size()      const noexcept;
    [[gnu::always_inline]] inline bool             empty()     const noexcept;

    void reserve(std::size_t n);
    [[gnu::hot]] void append(uint64_t timestamp, Side side, uint8_t level,
              Num price, Num quantity, Num orders);
    // Clear the vectors, but not the symbol
    void clear() noexcept;

    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint64_t>& timestamps()  const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Side>&     sides()       const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint8_t>&  levels()      const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      prices()      const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      quantities()  const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      orders()      const noexcept;

    /*
     * ASCII CSV-style representation with snake_case column names.
     *
     * Rows include symbol, numeric mode enum, timestamp, side, optional level,
     * price, quantity, orders. Floating-point columns use fixed format with
     * decimal_places passed to std::setprecision (ignored for non-floating Num).
     * Default precision is 6.
     *
     * Row indices are half-open: [start_row, end_row) into the appended tick rows
     * (same length as prices_size). When end_row is the maximum value_t of
     * std::size_t, end_row is treated as size().
     *
     * Invalid indices (including start_row > end_row or end_row > size()))
     * return an empty string.
     */
    [[nodiscard]] std::string to_string(
        const bool header = true,
        const unsigned int decimal_places = 6,
        std::size_t start_row = 0U,
        std::size_t end_row = std::numeric_limits<std::size_t>::max()) const;

    /*
     * Write data to a CSV file in chunks of chunk_size rows.
     * The first chunk includes the header; subsequent chunks omit it.
     * Returns false if the file cannot be opened, empty() or chunk_size == 0.
     */
    bool to_csv(
        const std::string& path,
        std::size_t chunk_size,
        unsigned int decimal_places = 6) const;

    /*
     * Read a CSV file produced by to_csv() and append each data row via
     * operator>>. The header line is skipped automatically. Lines that
     * fail to parse are skipped; a warning is printed to stderr with the
     * 1-based line number. Returns false only when the file cannot be opened.
     */
    bool load(const std::string& path);

    /*
     * Peek at the first data row of a CSV file and return {symbol, mode}.
     * Returns std::nullopt if the file cannot be opened, has no data rows,
     * or the mode field is not a recognised value.
     */
    static std::optional<std::pair<std::string, MarketDataMode>>
    peek_csv(const std::string& path);

    template<typename N>
    friend std::ostream& operator<<(std::ostream& os, const MarketData<N>& md);
    template<typename N>
    friend std::istream& operator>>(std::istream& is, MarketData<N>& md);

private:
    /*
     * Parse one CSV data row (no header) and call append().
     * Returns false if any field is malformed.
     */
    bool parse_row_(const std::string& line);

    std::string           symbol_;
    MarketDataMode        mode_      {MarketDataMode::ALL};
    uint8_t               max_level_ {4};
    std::vector<uint64_t> timestamps_;
    std::vector<Side>     sides_;
    std::vector<uint8_t>  levels_;
    std::vector<Num>      prices_;
    std::vector<Num>      quantities_;
    std::vector<Num>      orders_;
};

template<typename Num>
MarketData<Num>::MarketData(const std::string& symbol)
    : symbol_(symbol)
{
}

template<typename Num>
MarketData<Num>::MarketData(const std::string& symbol, MarketDataMode mode)
    : symbol_(symbol),
      mode_(mode),
      max_level_(mode == MarketDataMode::TRADE ? 0U : 4U)
{
}

template<typename Num>
MarketData<Num>::MarketData(const std::string& symbol, MarketDataMode mode, uint8_t max_level)
    : symbol_(symbol),
      mode_(mode),
      max_level_(max_level)
{
}

template<typename Num>
std::string_view MarketData<Num>::symbol() const noexcept
{
    return symbol_;
}

template<typename Num>
MarketDataMode MarketData<Num>::mode() const noexcept
{
    return mode_;
}

template<typename Num>
uint8_t MarketData<Num>::max_level() const noexcept
{
    return max_level_;
}

template<typename Num>
std::size_t MarketData<Num>::size() const noexcept
{
    return prices_.size();
}

template<typename Num>
[[gnu::always_inline]] bool MarketData<Num>::empty() const noexcept
{
    return prices_.empty();
}

template<typename Num>
void MarketData<Num>::reserve(std::size_t n)
{
    timestamps_.reserve(n);
    sides_.reserve(n);
    if (mode_ == MarketDataMode::ALL || mode_ == MarketDataMode::LIQUIDITY)
        levels_.reserve(n);
    prices_.reserve(n);
    quantities_.reserve(n);
    orders_.reserve(n);
}

template<typename Num>
void MarketData<Num>::append(uint64_t timestamp, Side side, uint8_t level,
                             Num price, Num quantity, Num orders)
{
    if (level > max_level_)
        return;
    timestamps_.push_back(timestamp);
    sides_.push_back(side);
    if (mode_ == MarketDataMode::ALL || mode_ == MarketDataMode::LIQUIDITY)
        levels_.push_back(level);
    prices_.push_back(price);
    quantities_.push_back(quantity);
    orders_.push_back(orders);
}

template<typename Num>
void MarketData<Num>::clear() noexcept
{
    timestamps_.clear();
    sides_.clear();
    levels_.clear();
    prices_.clear();
    quantities_.clear();
    orders_.clear();
}

template<typename Num>
const std::vector<uint64_t>& MarketData<Num>::timestamps() const noexcept
{
    return timestamps_;
}

template<typename Num>
const std::vector<Side>& MarketData<Num>::sides() const noexcept
{
    return sides_;
}

template<typename Num>
const std::vector<uint8_t>& MarketData<Num>::levels() const noexcept
{
    return levels_;
}

template<typename Num>
const std::vector<Num>& MarketData<Num>::prices() const noexcept
{
    return prices_;
}

template<typename Num>
const std::vector<Num>& MarketData<Num>::quantities() const noexcept
{
    return quantities_;
}

template<typename Num>
const std::vector<Num>& MarketData<Num>::orders() const noexcept
{
    return orders_;
}

template<typename Num>
std::string MarketData<Num>::to_string(const bool header, 
                                       const unsigned int decimal_places,
                                       const std::size_t start_row,
                                       const std::size_t end_row) const
{
    const std::size_t n = prices_.size();
    const std::size_t end_exclusive =
        (end_row == std::numeric_limits<std::size_t>::max()) ? n : end_row;

    if (start_row > end_exclusive || end_exclusive > n)
        return {};

    std::ostringstream oss;
    if constexpr (std::is_floating_point_v<Num>)
        oss << std::fixed << std::setprecision(static_cast<int>(decimal_places));

    
    if (header)
        oss << "symbol,mode,timestamp,side,level,price,quantity,orders\n";

    const bool has_levels = (levels_.size() == prices_.size());
    for (std::size_t idx = start_row; idx < end_exclusive; ++idx)
    {
        oss << symbol_ << ','
            << marketkernel::to_string(mode_) << ','
            << timestamps_[idx] << ','
            << marketkernel::to_string(sides_[idx]) << ',';
        if (has_levels)
            oss << static_cast<unsigned>(levels_[idx]);
        oss << ','
            << prices_[idx] << ','
            << quantities_[idx] << ','
            << orders_[idx] << '\n';
    }

    return oss.str();
}

template<typename Num>
bool MarketData<Num>::to_csv(const std::string& path,
                             const std::size_t chunk_size,
                             const unsigned int decimal_places) const
{
    const std::size_t n = prices_.size();
    if (n == 0 || chunk_size == 0)
        return false;

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        return false;

    std::size_t start = 0;
    bool header = true;
    while (start < n)
    {
        const std::size_t end = (chunk_size < n - start) ? start + chunk_size : n;
        file << to_string(header, decimal_places, start, end);
        header = false;
        start = end;
    }
    return file.good();
}

template<typename Num>
bool MarketData<Num>::parse_row_(const std::string& line)
{
    std::istringstream ss(line);
    std::string token;
    // symbol and mode columns are informational; the object owns its own symbol/mode
    if (!std::getline(ss, token,      ',')) return false;  // symbol
    if (!std::getline(ss, token,      ',')) return false;  // mode

    std::string ts_str;
    if (!std::getline(ss, ts_str,     ',')) return false;
    std::string side_str;
    if (!std::getline(ss, side_str,   ',')) return false;
    std::string level_str;
    if (!std::getline(ss, level_str,  ',')) return false;
    std::string price_str;
    if (!std::getline(ss, price_str,  ',')) return false;
    std::string qty_str;
    if (!std::getline(ss, qty_str,    ',')) return false;
    std::string orders_str;
    if (!std::getline(ss, orders_str))      return false;
    // strip Windows \r line endings
    if (!orders_str.empty() && orders_str.back() == '\r')
        orders_str.pop_back();

    uint64_t timestamp = 0U;
    try { timestamp = std::stoull(ts_str); } catch (...) { return false; }

    Side side;
    if      (side_str == "buy")  side = Side::BUY;
    else if (side_str == "sell") side = Side::SELL;
    else return false;

    uint8_t level = 0U;
    if (!level_str.empty())
    {
        try
        {
            const unsigned long lv = std::stoul(level_str);
            if (lv > 255UL) return false;
            level = static_cast<uint8_t>(lv);
        }
        catch (...) { return false; }
    }

    Num price{}, quantity{}, orders_val{};
    {
        std::istringstream sp(price_str);   sp >> price;      if (sp.fail()) return false;
        std::istringstream sq(qty_str);     sq >> quantity;   if (sq.fail()) return false;
        std::istringstream so(orders_str);  so >> orders_val; if (so.fail()) return false;
    }

    append(timestamp, side, level, price, quantity, orders_val);
    return true;
}

template<typename Num>
bool MarketData<Num>::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    std::size_t line_num = 0U;
    bool header_skipped = false;

    while (std::getline(file, line))
    {
        ++line_num;
        if (line.empty()) continue;
        if (!header_skipped)
        {
            header_skipped = true;
            continue;
        }
        std::istringstream ss(line);
        ss >> *this;
        if (ss.fail())
            std::cerr << "[MarketData::load] parse error on line " << line_num << '\n';
    }
    return true;
}

template<typename Num>
std::optional<std::pair<std::string, MarketDataMode>>
MarketData<Num>::peek_csv(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return std::nullopt;

    std::string line;
    // skip header
    if (!std::getline(file, line))
        return std::nullopt;
    // first data row
    if (!std::getline(file, line) || line.empty())
        return std::nullopt;
    // strip Windows \r
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::istringstream ss(line);
    std::string symbol_token, mode_token;
    if (!std::getline(ss, symbol_token, ',')) return std::nullopt;
    if (!std::getline(ss, mode_token,   ',')) return std::nullopt;

    MarketDataMode mode;
    if      (mode_token == "all")       mode = MarketDataMode::ALL;
    else if (mode_token == "trade")     mode = MarketDataMode::TRADE;
    else if (mode_token == "liquidity") mode = MarketDataMode::LIQUIDITY;
    else if (mode_token == "level")     mode = MarketDataMode::LEVEL;
    else return std::nullopt;

    return std::make_pair(symbol_token, mode);
}

template<typename Num>
std::ostream& operator<<(std::ostream& os, const MarketData<Num>& md)
{
    return os << md.to_string();
}

template<typename Num>
std::istream& operator>>(std::istream& is, MarketData<Num>& md)
{
    std::string row;
    if (std::getline(is, row))
    {
        if (!md.parse_row_(row))
            is.setstate(std::ios::failbit);
    }
    return is;
}

} // namespace marketkernel
