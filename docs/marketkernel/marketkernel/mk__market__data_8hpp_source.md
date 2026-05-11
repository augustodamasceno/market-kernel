

# File mk\_market\_data.hpp

[**File List**](files.md) **>** [**include**](dir_d44c64559bbebec7f509842c48db8b23.md) **>** [**mk\_market\_data.hpp**](mk__market__data_8hpp.md)

[Go to the documentation of this file](mk__market__data_8hpp.md)


```C++
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
    void clear() noexcept;

    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint64_t>& timestamps()  const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Side>&     sides()       const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<uint8_t>&  levels()      const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      prices()      const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      quantities()  const noexcept;
    [[gnu::always_inline, gnu::hot]] inline const std::vector<Num>&      orders()      const noexcept;

    [[nodiscard]] std::string to_string(
        const bool header = true,
        const unsigned int decimal_places = 6,
        std::size_t start_row = 0U,
        std::size_t end_row = std::numeric_limits<std::size_t>::max()) const;

    bool to_csv(
        const std::string& path,
        std::size_t chunk_size,
        unsigned int decimal_places = 6) const;

    bool from_csv(const std::string& path);

    static std::optional<std::pair<std::string, MarketDataMode>>
    peek_csv(const std::string& path);

    bool save_binary(const std::string& path) const;

    bool load_binary_mmap(const std::string& path);

    template<typename N>
    friend std::ostream& operator<<(std::ostream& os, const MarketData<N>& md);
    template<typename N>
    friend std::istream& operator>>(std::istream& is, MarketData<N>& md);

private:
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
```


