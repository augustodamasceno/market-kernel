/* Market Kernel : MarketData Class
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_market_data.hpp
 * @brief MarketData template class for storing and processing market ticks.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else  // POSIX: Linux, macOS, FreeBSD
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif  // _WIN32

#include "mk_market_data_mode.h"
#include "mk_side.h"

namespace marketkernel {

/**
 * @brief Structure-of-Arrays market tick container for low-latency bulk calculations.
 *
 * @details Each tick field is stored in its own contiguous vector so that
 * operations on a single field (e.g., iterating all prices) from_csv only
 * the relevant data into cache and are SIMD-friendly.
 *
 * - level == 0: trade (last traded price/quantity).
 * - level >= 1: orderbook depth (1 = top of book, ...).
 *
 * Call reserve() before the session starts to avoid reallocations.
 *
 * Mode controls which fields are stored per append() call:
 * - ALL: all fields stored, including level (default).
 * - TRADE: level ignored and not stored; suited for trade-only feeds.
 * - LIQUIDITY: all fields stored; semantically scoped to orderbook updates.
 * - LEVEL: level ignored and not stored; suited for depth snapshots.
 *
 * max_level controls the maximum accepted level value. append() is a no-op
 * when level > max_level. Default is 4.
 *
 * @tparam Num Numeric type for price, quantity, and orders
 *             (e.g., @c float, @c double, or a user-defined fixed-point type).
 */
template<typename Num>
class alignas(64) MarketData
{
public:
    /// @brief Default-constructs an empty container (symbol="", mode=ALL, max_level=4).
    MarketData() = default;
    /// @brief Construct with a symbol name; mode defaults to ALL, max_level to 4.
    /// @param symbol Instrument symbol (e.g., "es", "nvda").
    explicit MarketData(const std::string& symbol);
    /// @brief Construct with symbol and mode; max_level defaults to 4 (0 for TRADE).
    /// @param symbol Instrument symbol.
    /// @param mode   Storage mode controlling which fields are kept.
    MarketData(const std::string& symbol, MarketDataMode mode);
    /// @brief Construct with symbol, mode, and explicit max_level.
    /// @param symbol    Instrument symbol.
    /// @param mode      Storage mode.
    /// @param max_level Maximum accepted orderbook level; ticks above this are dropped.
    MarketData(const std::string& symbol, MarketDataMode mode, uint8_t max_level);

    /// @brief Return the instrument symbol.
    [[gnu::always_inline]] inline std::string_view symbol()    const noexcept;
    /// @brief Return the storage mode.
    [[gnu::always_inline]] inline MarketDataMode   mode()      const noexcept;
    /// @brief Return the maximum accepted orderbook level.
    [[gnu::always_inline]] inline uint8_t          max_level() const noexcept;
    /// @brief Return the number of stored ticks.
    [[gnu::always_inline]] inline std::size_t      size()      const noexcept;
    /// @brief Return true when no ticks have been stored.
    [[gnu::always_inline]] inline bool             empty()     const noexcept;

    /// @brief Pre-allocate capacity for @p n ticks in all active field vectors.
    /// @param n Number of ticks to reserve capacity for.
    void reserve(std::size_t n);
    /**
     * @brief Append one tick; silently drops the tick when level > max_level.
     * @param timestamp Nanosecond epoch timestamp.
     * @param side      Buy or sell side.
     * @param level     Orderbook depth level (0 = trade, >= 1 = depth).
     * @param price     Tick price.
     * @param quantity  Tick quantity.
     * @param orders    Number of orders at this price level.
     */
    [[gnu::hot]] void append(uint64_t timestamp, Side side, uint8_t level,
              Num price, Num quantity, Num orders);
    /// @brief Clear all tick vectors; symbol, mode, and max_level are preserved.
    void clear() noexcept;

    /// @brief Return a const reference to the timestamps vector.
    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint64_t>& timestamps()  const noexcept;
    /// @brief Return a const reference to the sides vector.
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Side>&     sides()       const noexcept;
    /// @brief Return a const reference to the levels vector (empty in TRADE/LEVEL mode).
    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint8_t>&  levels()      const noexcept;
    /// @brief Return a const reference to the prices vector.
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      prices()      const noexcept;
    /// @brief Return a const reference to the quantities vector.
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      quantities()  const noexcept;
    /// @brief Return a const reference to the orders vector.
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      orders()      const noexcept;

    /**
     * @brief Render ticks as an ASCII CSV string.
     *
     * @details Columns: symbol, mode, timestamp, side, level, price, quantity,
     * orders. Floating-point columns use fixed notation with @p decimal_places
     * digits (ignored for non-floating @c Num).
     *
     * Row range is half-open [start_row, end_row). When @p end_row equals
     * @c std::numeric_limits<std::size_t>::max() it is treated as size().
     * Returns an empty string for invalid indices.
     *
     * @param header         Include the header row when true (default: true).
     * @param decimal_places Floating-point precision (default: 6).
     * @param start_row      First row index, inclusive (default: 0).
     * @param end_row        Past-the-end row index (default: all rows).
     * @return CSV-formatted string, or empty string on invalid range.
     */
    [[nodiscard]] std::string to_string(
        const bool header = true,
        const unsigned int decimal_places = 6,
        std::size_t start_row = 0U,
        std::size_t end_row = std::numeric_limits<std::size_t>::max()) const;

    /**
     * @brief Write tick data to a CSV file in chunks of @p chunk_size rows.
     * @details The first chunk includes the header; subsequent chunks omit it.
     * @param path           Destination file path.
     * @param chunk_size     Number of rows per write chunk.
     * @param decimal_places Floating-point precision (default: 6).
     * @return false if the file cannot be opened, the container is empty(), or chunk_size == 0.
     */
    bool to_csv(
        const std::string& path,
        std::size_t chunk_size,
        unsigned int decimal_places = 6) const;

    /**
     * @brief Load ticks from a CSV file produced by to_csv().
     * @details The header line is skipped automatically. Lines that fail to
     * parse are skipped; a warning is printed to stderr with the 1-based line number.
     * @param path Path to the CSV file.
     * @return false only when the file cannot be opened.
     */
    bool from_csv(const std::string& path);

    /**
     * @brief Peek at the first data row of a CSV file without loading it.
     * @param path Path to the CSV file.
     * @return A pair {symbol, mode} from the first data row, or @c std::nullopt
     *         if the file cannot be opened, has no data rows, or the mode field
     *         is not a recognised value.
     */
    static std::optional<std::pair<std::string, MarketDataMode>>
    peek_csv(const std::string& path);

    /**
     * @brief Write a binary snapshot of all tick data to disk using block I/O.
     *
     * @details Serialises the container to a compact binary format optimised for
     * fast bulk reloads.  The file begins with a fixed 32-byte header:
     *
     * | Offset | Size | Field        | Notes                              |
     * |-------:|-----:|:-------------|:-----------------------------------|
     * |      0 |    4 | magic        | @c 'M','K','M','D'                 |
     * |      4 |    1 | version      | @c 1                               |
     * |      5 |    1 | sizeof(Num)  | validated on load                  |
     * |      6 |    1 | mode         | @c MarketDataMode cast to uint8_t  |
     * |      7 |    1 | max_level    | uint8_t                            |
     * |      8 |    8 | n_ticks      | uint64_t, native endian            |
     * |     16 |    8 | symbol_len   | uint64_t                           |
     * |     24 |    1 | has_levels   | @c 0 or @c 1                       |
     * |     25 |    7 | reserved     | zeros                              |
     *
     * Immediately after the header come @p symbol_len raw symbol bytes (no null
     * terminator), followed by the six tick arrays written sequentially:
     *   - @c timestamps  — @c uint64_t[n_ticks]
     *   - @c sides       — @c Side[n_ticks]
     *   - @c levels      — @c uint8_t[n_ticks]  (omitted when @c has_levels == 0)
     *   - @c prices      — @c Num[n_ticks]
     *   - @c quantities  — @c Num[n_ticks]
     *   - @c orders      — @c Num[n_ticks]
     *
     * Each array is written in a single @c std::fwrite call, so I/O overhead
     * is proportional to the number of arrays, not the number of ticks.
     *
     * The resulting file can be reloaded with load_binary_mmap().
     *
     * @note The file is written in the native byte order of the host.
     *       Files are not portable across architectures with different endianness.
     * @note Not thread-safe: do not call concurrently with append() or clear().
     *
     * @param path Destination file path.  An existing file is overwritten.
     * @return @c true on success.
     * @return @c false if the container is empty(), the file cannot be opened,
     *         or any write call fails.
     */
    bool save_binary(const std::string& path) const;

    /**
     * @brief Load a binary snapshot via memory-mapped I/O for zero-copy ingestion.
     *
     * @details Maps the file produced by save_binary() directly into the process
     * address space using the OS memory-mapping facility.  Where supported, the
     * implementation requests huge / large pages to reduce TLB pressure when
     * loading large datasets:
     *
     *   - **Linux** — POSIX @c mmap(2) with @c MAP_PRIVATE.  After mapping,
     *     @c madvise(2) with @c MADV_HUGEPAGE requests Transparent Huge Page
     *     (THP) promotion (2 MB pages), and @c MADV_SEQUENTIAL enables
     *     kernel read-ahead.  @c MADV_HUGEPAGE is compiled out on platforms
     *     that do not define it (macOS, older FreeBSD).
     *   - **macOS / FreeBSD** — same @c mmap(2) path; @c MADV_SEQUENTIAL is
     *     applied; huge-page hints are omitted where unavailable.
     *   - **Windows** — @c CreateFileMapping is first attempted with
     *     @c SEC_LARGE_PAGES (4 MB pages on x86-64) and @c MapViewOfFile with
     *     @c FILE_MAP_LARGE_PAGES.  Both require @c SeLockMemoryPrivilege; if
     *     that privilege is not held the calls fail and the implementation
     *     retries transparently with standard 4 KB pages.
     *
     * In all cases the kernel maps file data directly from the page cache,
     * eliminating the kernel-buffer-to-user-buffer copy of @c read() / @c fread().
     *
     * On entry, the method validates:
     *   -# Magic bytes @c 'M','K','M','D' at offset 0.
     *   -# Version byte equals @c 1.
     *   -# @c sizeof(Num) byte matches the @c Num of this instantiation;
     *      attempting to load a @c double file into a @c float instance (or
     *      vice-versa) will be rejected here.
     *   -# File is large enough to hold all declared arrays.
     *
     * On success the existing contents of the container are replaced by the
     * loaded data; symbol_, mode_, and max_level_ are overwritten from the header.
     * On failure the container state is unchanged.
     *
     * @note The file is assumed to be in native byte order.
     *       Files produced on a big-endian host cannot be read on a little-endian
     *       host and vice-versa.
     * @note Not thread-safe: do not call concurrently with any other method.
     *
     * @param path Path to a binary file produced by save_binary().
     * @return @c true on success.
     * @return @c false if the file cannot be opened, the header magic or version
     *         is wrong, @c sizeof(Num) mismatches, or the file is truncated.
     */
    bool load_binary_mmap(const std::string& path);

    /// @brief Serialize all ticks as CSV to an output stream.
    template<typename N>
    friend std::ostream& operator<<(std::ostream& os, const MarketData<N>& md);
    /// @brief Deserialize one CSV row from an input stream and append it.
    template<typename N>
    friend std::istream& operator>>(std::istream& is, MarketData<N>& md);

private:
    /**
     * @brief Parse one CSV data row (no header) and call append().
     * @param line A single CSV data row string.
     * @return false if any field is malformed.
     */
    bool parse_row_(const std::string& line);

    std::string           symbol_;                          ///< Instrument symbol.
    MarketDataMode        mode_      {MarketDataMode::ALL}; ///< Active storage mode.
    uint8_t               max_level_ {4};                   ///< Maximum accepted orderbook level.
    std::vector<uint64_t> timestamps_;                      ///< Nanosecond epoch timestamps.
    std::vector<Side>     sides_;                           ///< Buy/sell sides.
    std::vector<uint8_t>  levels_;                          ///< Orderbook depth levels (empty in TRADE/LEVEL mode).
    std::vector<Num>      prices_;                          ///< Tick prices.
    std::vector<Num>      quantities_;                      ///< Tick quantities.
    std::vector<Num>      orders_;                          ///< Number of orders at the price level.
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
bool MarketData<Num>::from_csv(const std::string& path)
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
            std::cerr << "[MarketData::from_csv] parse error on line " << line_num << '\n';
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

template<typename Num>
bool MarketData<Num>::save_binary(const std::string& path) const
{
    const std::size_t n = prices_.size();
    if (n == 0)
        return false;

    std::FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp)
        return false;

    const uint8_t  magic[4]     = {'M', 'K', 'M', 'D'};
    const uint8_t  version      = 1U;
    const uint8_t  sizeof_num   = static_cast<uint8_t>(sizeof(Num));
    const uint8_t  mode_val     = static_cast<uint8_t>(mode_);
    const uint64_t n_ticks      = static_cast<uint64_t>(n);
    const uint64_t sym_len      = static_cast<uint64_t>(symbol_.size());
    const uint8_t  has_levels   = (levels_.size() == n) ? 1U : 0U;
    const uint8_t  reserved[7]  = {};

    bool ok = true;
    ok = ok && (std::fwrite(magic,        1,              4,  fp) == 4);
    ok = ok && (std::fwrite(&version,     1,              1,  fp) == 1);
    ok = ok && (std::fwrite(&sizeof_num,  1,              1,  fp) == 1);
    ok = ok && (std::fwrite(&mode_val,    1,              1,  fp) == 1);
    ok = ok && (std::fwrite(&max_level_,  1,              1,  fp) == 1);
    ok = ok && (std::fwrite(&n_ticks,     sizeof(n_ticks), 1, fp) == 1);
    ok = ok && (std::fwrite(&sym_len,     sizeof(sym_len), 1, fp) == 1);
    ok = ok && (std::fwrite(&has_levels,  1,              1,  fp) == 1);
    ok = ok && (std::fwrite(reserved,     1,              7,  fp) == 7);

    if (!symbol_.empty())
        ok = ok && (std::fwrite(symbol_.data(), 1, symbol_.size(), fp) == symbol_.size());

    ok = ok && (std::fwrite(timestamps_.data(), sizeof(uint64_t), n, fp) == n);
    ok = ok && (std::fwrite(sides_.data(),      sizeof(Side),     n, fp) == n);
    if (has_levels)
        ok = ok && (std::fwrite(levels_.data(), sizeof(uint8_t),  n, fp) == n);
    ok = ok && (std::fwrite(prices_.data(),     sizeof(Num),      n, fp) == n);
    ok = ok && (std::fwrite(quantities_.data(), sizeof(Num),      n, fp) == n);
    ok = ok && (std::fwrite(orders_.data(),     sizeof(Num),      n, fp) == n);

    std::fclose(fp);
    return ok;
}

template<typename Num>
bool MarketData<Num>::load_binary_mmap(const std::string& path)
{
    const uint8_t* base      = nullptr;
    std::size_t    file_size = 0U;

#ifdef _WIN32
    HANDLE hFile = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER li{};
    if (!::GetFileSizeEx(hFile, &li) || li.QuadPart < 32)
    {
        ::CloseHandle(hFile);
        return false;
    }
    file_size = static_cast<std::size_t>(li.QuadPart);

    // Attempt large-page file mapping (4 MB pages on x86-64).
    // SEC_LARGE_PAGES / FILE_MAP_LARGE_PAGES require SeLockMemoryPrivilege;
    // fall back to standard 4 KB pages silently when the privilege is absent.
    HANDLE hMap = ::CreateFileMappingA(hFile, nullptr,
                                       PAGE_READONLY | SEC_LARGE_PAGES, 0, 0, nullptr);
    const bool large_pages = (hMap != nullptr);
    if (!hMap)
        hMap = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    ::CloseHandle(hFile);
    if (!hMap)
        return false;

    const DWORD view_flags = large_pages
        ? (FILE_MAP_READ | FILE_MAP_LARGE_PAGES)
        : FILE_MAP_READ;
    const void* const raw = ::MapViewOfFile(hMap, view_flags, 0, 0, 0);
    ::CloseHandle(hMap);
    if (!raw)
        return false;

    base = static_cast<const uint8_t*>(raw);

#else  // POSIX: Linux, macOS, FreeBSD

    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    struct stat st{};
    if (::fstat(fd, &st) != 0 || st.st_size < 32)
    {
        ::close(fd);
        return false;
    }
    file_size = static_cast<std::size_t>(st.st_size);

    // mmap maps the file's page-cache pages directly into the process address
    // space, eliminating the kernel-buffer-to-user-buffer copy of read().
    void* const ptr = ::mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (ptr == MAP_FAILED)
        return false;

    // Request Transparent Huge Page (THP) promotion (2 MB pages on x86-64)
    // to reduce TLB pressure on large datasets.  Guarded because macOS and
    // older BSDs do not define MADV_HUGEPAGE.
#ifdef MADV_HUGEPAGE
    ::madvise(ptr, file_size, MADV_HUGEPAGE);
#endif
    ::madvise(ptr, file_size, MADV_SEQUENTIAL);
    base = static_cast<const uint8_t*>(ptr);

#endif  // _WIN32 / POSIX

    // Validate header.
    const uint8_t expected_magic[4] = {'M', 'K', 'M', 'D'};
    const bool header_ok =
        file_size >= 32U
        && std::memcmp(base, expected_magic, 4) == 0
        && base[4] == 1U
        && base[5] == static_cast<uint8_t>(sizeof(Num));

    if (!header_ok)
    {
#ifdef _WIN32
        ::UnmapViewOfFile(raw);
#else  // POSIX
        ::munmap(ptr, file_size);
#endif  // _WIN32
        return false;
    }

    const uint8_t mode_val   = base[6];
    const uint8_t max_lv     = base[7];
    uint64_t n_ticks = 0U;
    uint64_t sym_len = 0U;
    std::memcpy(&n_ticks, base + 8,  sizeof(uint64_t));
    std::memcpy(&sym_len, base + 16, sizeof(uint64_t));
    const uint8_t has_levels = base[24];

    const std::size_t data_offset  = 32U + static_cast<std::size_t>(sym_len);
    const std::size_t levels_bytes = has_levels ? static_cast<std::size_t>(n_ticks) : 0U;
    const std::size_t expected =
          data_offset
        + n_ticks * sizeof(uint64_t)
        + n_ticks * sizeof(Side)
        + levels_bytes
        + n_ticks * sizeof(Num) * 3U;

    if (expected > file_size)
    {
#ifdef _WIN32
        ::UnmapViewOfFile(raw);
#else  // POSIX
        ::munmap(ptr, file_size);
#endif  // _WIN32
        return false;
    }

    // Populate vectors directly from the mapped pages.
    symbol_.assign(reinterpret_cast<const char*>(base + 32),
                   static_cast<std::size_t>(sym_len));
    mode_      = static_cast<MarketDataMode>(mode_val);
    max_level_ = max_lv;

    const uint8_t* d = base + data_offset;

    timestamps_.assign(reinterpret_cast<const uint64_t*>(d),
                       reinterpret_cast<const uint64_t*>(d) + n_ticks);
    d += n_ticks * sizeof(uint64_t);

    sides_.assign(reinterpret_cast<const Side*>(d),
                  reinterpret_cast<const Side*>(d) + n_ticks);
    d += n_ticks * sizeof(Side);

    if (has_levels)
    {
        levels_.assign(d, d + n_ticks);
        d += n_ticks;
    }
    else
    {
        levels_.clear();
    }

    prices_.assign(reinterpret_cast<const Num*>(d),
                   reinterpret_cast<const Num*>(d) + n_ticks);
    d += n_ticks * sizeof(Num);

    quantities_.assign(reinterpret_cast<const Num*>(d),
                       reinterpret_cast<const Num*>(d) + n_ticks);
    d += n_ticks * sizeof(Num);

    orders_.assign(reinterpret_cast<const Num*>(d),
                   reinterpret_cast<const Num*>(d) + n_ticks);

#ifdef _WIN32
    ::UnmapViewOfFile(raw);
#else  // POSIX
    ::munmap(ptr, file_size);
#endif  // _WIN32
    return true;
}

} // namespace marketkernel
