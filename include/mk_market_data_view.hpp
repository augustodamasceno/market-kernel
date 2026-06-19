/* Market Kernel : MarketDataView Class
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

/**
 * @file mk_market_data_view.hpp
 * @brief Zero-copy, read-only view over a memory-mapped MarketData binary file.
 *
 * @details MarketDataView<Num> maps a binary file produced by
 * MarketData<Num>::save_binary() directly into the process address space and
 * exposes every tick array as a Span<T> — a lightweight pointer/count pair
 * with full iterator support.  No tick data is ever copied to the heap;
 * load time is strictly $O(1)$ regardless of file size.  The OS handles
 * demand paging transparently as the backtester iterates the spans.
 *
 * The view is non-copyable but movable, following strict RAII ownership.
 * All data accessors remain valid for the lifetime of the view object.
 *
 * Template algorithms that consume MarketData<Num> can also consume
 * MarketDataView<Num> without modification: both types expose the same
 * named accessor set (symbol(), mode(), size(), prices(), quantities(), etc.)
 * returning iterable range objects.
 *
 * @par Platform support
 * - Linux / macOS / FreeBSD — POSIX mmap(2) with MADV_SEQUENTIAL read-ahead.
 *   MADV_HUGEPAGE (2 MB THP promotion) is requested on Linux where available.
 * - Windows — CreateFileMapping + MapViewOfFile with large-page fallback.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

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
 * @brief Lightweight, non-owning, read-only contiguous range over elements of type @p T.
 *
 * @details Acts as a C++17-compatible substitute for @c std::span<const T>
 * (introduced in C++20).  Provides the same iterator interface so that template
 * algorithms can consume @c Span<T> and @c std::vector<T> interchangeably via
 * range-based for loops, @c .data(), @c .size(), and @c operator[].
 *
 * @tparam T Element type.  Must be a trivially copyable, standard-layout type.
 *
 * @note Time Complexity: all methods are $O(1)$.
 * @note Memory: zero heap allocations.
 */
template<typename T>
struct Span
{
    const T*    ptr;   ///< Pointer to the first element (nullptr when count == 0).
    std::size_t count; ///< Number of elements.

    /// @brief Return a pointer to the first element.
    [[gnu::always_inline]] inline const T* data() const noexcept { return ptr; }
    /// @brief Return the number of elements.
    [[gnu::always_inline]] inline std::size_t size() const noexcept { return count; }
    /// @brief Return true when the span contains no elements.
    [[gnu::always_inline]] inline bool empty() const noexcept { return count == 0U; }
    /// @brief Return an iterator to the first element.
    [[gnu::always_inline]] inline const T* begin() const noexcept { return ptr; }
    /// @brief Return a past-the-end iterator.
    [[gnu::always_inline]] inline const T* end() const noexcept { return ptr + count; }
    /// @brief Subscript operator — no bounds checking.
    [[gnu::always_inline]] inline const T& operator[](std::size_t i) const noexcept
    {
        return ptr[i];
    }
};

/**
 * @brief Zero-copy, read-only view over a memory-mapped MarketData binary file.
 *
 * @details Owns the OS memory-mapping handle and exposes every tick array as a
 * Span<T> pointing directly into the mapped pages.  Load time is strictly
 * $O(1)$ — no data is copied to the heap; the OS demand-pages file content on
 * first access.
 *
 * The view is non-copyable (unique OS handle) but movable.  Move transfers
 * the mapping handle; the moved-from object becomes invalid (valid() == false).
 *
 * @par Template Interoperability
 * Any template algorithm parameterised over @c MarketData<Num> can accept
 * @c MarketDataView<Num> without modification, because both expose the same
 * accessor signatures returning iterable range objects.
 *
 * @par Usage
 * @code
 * marketkernel::MarketDataView<double> view("es_2026.mkmd");
 * if (!view.valid()) { // handle open failure }
 * for (double price : view.prices()) { // iterate zero-copy }
 * @endcode
 *
 * @tparam Num Numeric type used for price, quantity, and orders
 *             (must match the @c Num used when the file was saved).
 */
template<typename Num>
class alignas(64) MarketDataView
{
public:
    /**
     * @brief Default-constructs an empty, invalid view (no file is mapped).
     * @post valid() == false.
     *
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    MarketDataView() noexcept = default;

    /**
     * @brief Map @p path into the process address space.
     *
     * @details Opens the file, establishes the OS memory mapping, validates
     * the binary header (magic bytes, version, @c sizeof(Num)), and computes
     * direct typed pointers into the mapped region for every tick array.
     * On Linux, requests THP promotion (MADV_HUGEPAGE) and sequential
     * read-ahead (MADV_SEQUENTIAL) to reduce TLB pressure on large datasets.
     *
     * @pre @p path must be a file produced by @c MarketData<Num>::save_binary().
     * @post valid() == true on success; valid() == false on any failure.
     *
     * @param path Path to a binary MarketData snapshot.
     *
     * @note Time Complexity: $O(1)$ — no tick data is copied.
     * @note Memory: zero heap allocations for tick data.
     * @note Thread-safety: not thread-safe; do not construct concurrently over
     *       the same path without external synchronisation.
     */
    explicit MarketDataView(const std::string& path) noexcept;

    /**
     * @brief Destructor; unmaps the file and releases all OS resources.
     * @note Time Complexity: $O(1)$.
     */
    ~MarketDataView() noexcept;

    /// @brief MarketDataView is not copyable (unique OS mapping handle).
    MarketDataView(const MarketDataView&)            = delete;
    /// @brief MarketDataView is not copy-assignable.
    MarketDataView& operator=(const MarketDataView&) = delete;

    /**
     * @brief Move-constructs by transferring the mapping handle from @p other.
     * @post other.valid() == false.
     * @note Time Complexity: $O(1)$.
     */
    MarketDataView(MarketDataView&& other) noexcept;

    /**
     * @brief Move-assigns by transferring the mapping handle from @p other.
     * @post other.valid() == false.
     * @note Time Complexity: $O(1)$.
     */
    MarketDataView& operator=(MarketDataView&& other) noexcept;

    /**
     * @brief Return true when the file was mapped and validated successfully.
     * @return true  File is mapped; all data accessors are usable.
     * @return false Construction failed; all accessors return empty spans.
     */
    [[gnu::always_inline]] inline bool valid() const noexcept;

    /// @brief Return the instrument symbol read from the file header.
    [[gnu::always_inline]] inline std::string_view symbol()    const noexcept;
    /// @brief Return the storage mode read from the file header.
    [[gnu::always_inline]] inline MarketDataMode   mode()      const noexcept;
    /// @brief Return the maximum accepted orderbook level from the file header.
    [[gnu::always_inline]] inline uint8_t          max_level() const noexcept;
    /// @brief Return the number of ticks stored in the file.
    [[gnu::always_inline]] inline std::size_t      size()      const noexcept;
    /// @brief Return true when no ticks are stored in the file.
    [[gnu::always_inline]] inline bool             empty()     const noexcept;

    /**
     * @brief Return a zero-copy span over the nanosecond timestamp array.
     * @return Span<uint64_t> pointing directly into the mapped pages.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<uint64_t> timestamps()  const noexcept;

    /**
     * @brief Return a zero-copy span over the side array.
     * @return Span<Side> pointing directly into the mapped pages.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<Side>     sides()       const noexcept;

    /**
     * @brief Return a zero-copy span over the orderbook level array.
     * @details Returns an empty span when the file was saved in TRADE or LEVEL
     *          mode (level data was not stored).
     * @return Span<uint8_t> pointing into the mapped pages, or an empty span.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<uint8_t>  levels()      const noexcept;

    /**
     * @brief Return a zero-copy span over the price array.
     * @return Span<Num> pointing directly into the mapped pages.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<Num>      prices()      const noexcept;

    /**
     * @brief Return a zero-copy span over the quantity array.
     * @return Span<Num> pointing directly into the mapped pages.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<Num>      quantities()  const noexcept;

    /**
     * @brief Return a zero-copy span over the orders array.
     * @return Span<Num> pointing directly into the mapped pages.
     * @note Time Complexity: $O(1)$.
     * @note Memory: zero heap allocations.
     */
    [[gnu::always_inline, gnu::hot]] inline Span<Num>      orders()      const noexcept;

private:
    /**
     * @brief Release the OS mapping and reset every member to its default state.
     * @post valid() == false.
     * @note Time Complexity: $O(1)$.
     */
    void unmap_() noexcept;

    // --- OS mapping state (platform-specific) ---
#ifdef _WIN32
    void* map_view_ {nullptr}; ///< MapViewOfFile result (Windows).
#else
    void*       map_ptr_  {nullptr}; ///< mmap(2) base pointer (POSIX).
    std::size_t map_size_ {0U};      ///< Mapped region size in bytes (POSIX).
#endif

    const uint8_t* base_ {nullptr}; ///< Non-null iff the mapping is valid and verified.

    // --- File header metadata ---
    std::string_view symbol_;                          ///< Points into the mapped symbol bytes.
    MarketDataMode   mode_      {MarketDataMode::ALL}; ///< Storage mode from the file header.
    uint8_t          max_level_ {4U};                  ///< Maximum orderbook level from header.
    std::size_t      n_ticks_   {0U};                  ///< Number of ticks.

    // --- Zero-copy pointers into the mapped pages ---
    const uint64_t* ts_ptr_         {nullptr}; ///< Timestamps array start.
    const Side*     sides_ptr_      {nullptr}; ///< Sides array start.
    const uint8_t*  levels_ptr_     {nullptr}; ///< Levels array start (nullptr when absent).
    const Num*      prices_ptr_     {nullptr}; ///< Prices array start.
    const Num*      quantities_ptr_ {nullptr}; ///< Quantities array start.
    const Num*      orders_ptr_     {nullptr}; ///< Orders array start.
};

// ============================================================================
// Implementation
// ============================================================================

template<typename Num>
MarketDataView<Num>::MarketDataView(const std::string& path) noexcept
{
    std::size_t file_size = 0U;

#ifdef _WIN32
    HANDLE hFile = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    LARGE_INTEGER li{};
    if (!::GetFileSizeEx(hFile, &li) || li.QuadPart < 32)
    {
        ::CloseHandle(hFile);
        return;
    }
    file_size = static_cast<std::size_t>(li.QuadPart);

    HANDLE hMap = ::CreateFileMappingA(hFile, nullptr,
                                       PAGE_READONLY | SEC_LARGE_PAGES, 0, 0, nullptr);
    const bool large_pages = (hMap != nullptr);
    if (!hMap)
        hMap = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    ::CloseHandle(hFile);
    if (!hMap)
        return;

    const DWORD view_flags = large_pages
        ? (FILE_MAP_READ | FILE_MAP_LARGE_PAGES)
        : FILE_MAP_READ;
    void* const raw = ::MapViewOfFile(hMap, view_flags, 0, 0, 0);
    ::CloseHandle(hMap);
    if (!raw)
        return;

    map_view_ = raw;

#else  // POSIX: Linux, macOS, FreeBSD

    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return;

    struct stat st{};
    if (::fstat(fd, &st) != 0 || st.st_size < 32)
    {
        ::close(fd);
        return;
    }
    file_size = static_cast<std::size_t>(st.st_size);

    void* const ptr = ::mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (ptr == MAP_FAILED)
        return;

    // Commit mapping handles before any validation so unmap_() can clean up.
    map_ptr_  = ptr;
    map_size_ = file_size;

#ifdef MADV_HUGEPAGE
    ::madvise(ptr, file_size, MADV_HUGEPAGE);
#endif
    ::madvise(ptr, file_size, MADV_SEQUENTIAL);

#endif  // _WIN32 / POSIX

#ifdef _WIN32
    const uint8_t* const base = static_cast<const uint8_t*>(raw);
#else
    const uint8_t* const base = static_cast<const uint8_t*>(ptr);
#endif

    // Validate header: magic, version, sizeof(Num).
    const uint8_t expected_magic[4] = {'M', 'K', 'M', 'D'};
    if (file_size < 32U
        || std::memcmp(base, expected_magic, 4) != 0
        || base[4] != 1U
        || base[5] != static_cast<uint8_t>(sizeof(Num)))
    {
        unmap_();
        return;
    }

    const uint8_t mode_val   = base[6];
    const uint8_t max_lv     = base[7];
    uint64_t      n_ticks    = 0U;
    uint64_t      sym_len    = 0U;
    std::memcpy(&n_ticks, base + 8,  sizeof(uint64_t));
    std::memcpy(&sym_len, base + 16, sizeof(uint64_t));
    const uint8_t has_levels = base[24];

    const std::size_t data_offset  = 32U + static_cast<std::size_t>(sym_len);
    const std::size_t levels_bytes = has_levels ? static_cast<std::size_t>(n_ticks) : 0U;
    const std::size_t expected_size =
          data_offset
        + n_ticks * sizeof(uint64_t)
        + n_ticks * sizeof(Side)
        + levels_bytes
        + n_ticks * sizeof(Num) * 3U;

    if (expected_size > file_size)
    {
        unmap_();
        return;
    }

    // All checks passed - populate members.
    base_      = base;
    mode_      = static_cast<MarketDataMode>(mode_val);
    max_level_ = max_lv;
    n_ticks_   = static_cast<std::size_t>(n_ticks);
    symbol_    = std::string_view(reinterpret_cast<const char*>(base + 32),
                                  static_cast<std::size_t>(sym_len));

    const uint8_t* d = base + data_offset;

    ts_ptr_    = reinterpret_cast<const uint64_t*>(d);
    d         += n_ticks_ * sizeof(uint64_t);

    sides_ptr_ = reinterpret_cast<const Side*>(d);
    d         += n_ticks_ * sizeof(Side);

    if (has_levels)
    {
        levels_ptr_ = d;
        d          += n_ticks_;
    }

    prices_ptr_     = reinterpret_cast<const Num*>(d);
    d              += n_ticks_ * sizeof(Num);

    quantities_ptr_ = reinterpret_cast<const Num*>(d);
    d              += n_ticks_ * sizeof(Num);

    orders_ptr_     = reinterpret_cast<const Num*>(d);
}

template<typename Num>
MarketDataView<Num>::~MarketDataView() noexcept
{
    unmap_();
}

template<typename Num>
void MarketDataView<Num>::unmap_() noexcept
{
#ifdef _WIN32
    if (map_view_)
    {
        ::UnmapViewOfFile(map_view_);
        map_view_ = nullptr;
    }
#else
    if (map_ptr_ && map_size_ > 0U)
    {
        ::munmap(map_ptr_, map_size_);
        map_ptr_  = nullptr;
        map_size_ = 0U;
    }
#endif
    base_           = nullptr;
    symbol_         = {};
    n_ticks_        = 0U;
    ts_ptr_         = nullptr;
    sides_ptr_      = nullptr;
    levels_ptr_     = nullptr;
    prices_ptr_     = nullptr;
    quantities_ptr_ = nullptr;
    orders_ptr_     = nullptr;
}

template<typename Num>
MarketDataView<Num>::MarketDataView(MarketDataView&& other) noexcept
{
#ifdef _WIN32
    map_view_       = other.map_view_;
    other.map_view_ = nullptr;
#else
    map_ptr_        = other.map_ptr_;
    map_size_       = other.map_size_;
    other.map_ptr_  = nullptr;
    other.map_size_ = 0U;
#endif
    base_               = other.base_;
    symbol_             = other.symbol_;
    mode_               = other.mode_;
    max_level_          = other.max_level_;
    n_ticks_            = other.n_ticks_;
    ts_ptr_             = other.ts_ptr_;
    sides_ptr_          = other.sides_ptr_;
    levels_ptr_         = other.levels_ptr_;
    prices_ptr_         = other.prices_ptr_;
    quantities_ptr_     = other.quantities_ptr_;
    orders_ptr_         = other.orders_ptr_;

    other.base_             = nullptr;
    other.symbol_           = {};
    other.n_ticks_          = 0U;
    other.ts_ptr_           = nullptr;
    other.sides_ptr_        = nullptr;
    other.levels_ptr_       = nullptr;
    other.prices_ptr_       = nullptr;
    other.quantities_ptr_   = nullptr;
    other.orders_ptr_       = nullptr;
}

template<typename Num>
MarketDataView<Num>& MarketDataView<Num>::operator=(MarketDataView&& other) noexcept
{
    if (this != &other)
    {
        unmap_();
#ifdef _WIN32
        map_view_       = other.map_view_;
        other.map_view_ = nullptr;
#else
        map_ptr_        = other.map_ptr_;
        map_size_       = other.map_size_;
        other.map_ptr_  = nullptr;
        other.map_size_ = 0U;
#endif
        base_               = other.base_;
        symbol_             = other.symbol_;
        mode_               = other.mode_;
        max_level_          = other.max_level_;
        n_ticks_            = other.n_ticks_;
        ts_ptr_             = other.ts_ptr_;
        sides_ptr_          = other.sides_ptr_;
        levels_ptr_         = other.levels_ptr_;
        prices_ptr_         = other.prices_ptr_;
        quantities_ptr_     = other.quantities_ptr_;
        orders_ptr_         = other.orders_ptr_;

        other.base_             = nullptr;
        other.symbol_           = {};
        other.n_ticks_          = 0U;
        other.ts_ptr_           = nullptr;
        other.sides_ptr_        = nullptr;
        other.levels_ptr_       = nullptr;
        other.prices_ptr_       = nullptr;
        other.quantities_ptr_   = nullptr;
        other.orders_ptr_       = nullptr;
    }
    return *this;
}

template<typename Num>
bool MarketDataView<Num>::valid() const noexcept
{
    return base_ != nullptr;
}

template<typename Num>
std::string_view MarketDataView<Num>::symbol() const noexcept
{
    return symbol_;
}

template<typename Num>
MarketDataMode MarketDataView<Num>::mode() const noexcept
{
    return mode_;
}

template<typename Num>
uint8_t MarketDataView<Num>::max_level() const noexcept
{
    return max_level_;
}

template<typename Num>
std::size_t MarketDataView<Num>::size() const noexcept
{
    return n_ticks_;
}

template<typename Num>
bool MarketDataView<Num>::empty() const noexcept
{
    return n_ticks_ == 0U;
}

template<typename Num>
Span<uint64_t> MarketDataView<Num>::timestamps() const noexcept
{
    return {ts_ptr_, n_ticks_};
}

template<typename Num>
Span<Side> MarketDataView<Num>::sides() const noexcept
{
    return {sides_ptr_, n_ticks_};
}

template<typename Num>
Span<uint8_t> MarketDataView<Num>::levels() const noexcept
{
    return {levels_ptr_, levels_ptr_ ? n_ticks_ : 0U};
}

template<typename Num>
Span<Num> MarketDataView<Num>::prices() const noexcept
{
    return {prices_ptr_, n_ticks_};
}

template<typename Num>
Span<Num> MarketDataView<Num>::quantities() const noexcept
{
    return {quantities_ptr_, n_ticks_};
}

template<typename Num>
Span<Num> MarketDataView<Num>::orders() const noexcept
{
    return {orders_ptr_, n_ticks_};
}

} // namespace marketkernel
