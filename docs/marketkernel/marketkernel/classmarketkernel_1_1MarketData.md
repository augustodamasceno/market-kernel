

# Class marketkernel::MarketData

**template &lt;typename Num&gt;**



[**ClassList**](annotated.md) **>** [**marketkernel**](namespacemarketkernel.md) **>** [**MarketData**](classmarketkernel_1_1MarketData.md)



_Structure-of-Arrays market tick container for low-latency bulk calculations._ [More...](#detailed-description)

* `#include <mk_market_data.hpp>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**MarketData**](#function-marketdata-14) () = default<br>_Default-constructs an empty container (symbol="", mode=ALL, max\_level=4)._  |
|   | [**MarketData**](#function-marketdata-24) (const std::string & symbol) <br>_Construct with a symbol name; mode defaults to ALL, max\_level to 4._  |
|   | [**MarketData**](#function-marketdata-34) (const std::string & symbol, [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) mode) <br>_Construct with symbol and mode; max\_level defaults to 4 (0 for TRADE)._  |
|   | [**MarketData**](#function-marketdata-44) (const std::string & symbol, [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) mode, uint8\_t max\_level) <br>_Construct with symbol, mode, and explicit max\_level._  |
|  void | [**append**](#function-append) (uint64\_t timestamp, [**Side**](namespacemarketkernel.md#enum-side) side, uint8\_t level, Num price, Num quantity, Num orders) <br>_Append one tick; silently drops the tick when level &gt; max\_level._  |
|  void | [**clear**](#function-clear) () noexcept<br>_Clear all tick vectors; symbol, mode, and max\_level are preserved._  |
|  bool | [**empty**](#function-empty) () noexcept const<br>_Return true when no ticks have been stored._  |
|  bool | [**from\_csv**](#function-from_csv) (const std::string & path) <br>_Load ticks from a CSV file produced by_ [_**to\_csv()**_](classmarketkernel_1_1MarketData.md#function-to_csv) _._ |
|  const std::vector&lt; uint8\_t &gt; & | [**levels**](#function-levels) () noexcept const<br>_Return a const reference to the levels vector (empty in TRADE/LEVEL mode)._  |
|  bool | [**load\_binary\_mmap**](#function-load_binary_mmap) (const std::string & path) <br>_Load a binary snapshot via memory-mapped I/O for zero-copy ingestion._  |
|  uint8\_t | [**max\_level**](#function-max_level) () noexcept const<br>_Return the maximum accepted orderbook level._  |
|  [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) | [**mode**](#function-mode) () noexcept const<br>_Return the storage mode._  |
|  const std::vector&lt; Num &gt; & | [**orders**](#function-orders) () noexcept const<br>_Return a const reference to the orders vector._  |
|  const std::vector&lt; Num &gt; & | [**prices**](#function-prices) () noexcept const<br>_Return a const reference to the prices vector._  |
|  const std::vector&lt; Num &gt; & | [**quantities**](#function-quantities) () noexcept const<br>_Return a const reference to the quantities vector._  |
|  void | [**reserve**](#function-reserve) (std::size\_t n) <br>_Pre-allocate capacity for_ `n` _ticks in all active field vectors._ |
|  bool | [**save\_binary**](#function-save_binary) (const std::string & path) const<br>_Write a binary snapshot of all tick data to disk using block I/O._  |
|  const std::vector&lt; [**Side**](namespacemarketkernel.md#enum-side) &gt; & | [**sides**](#function-sides) () noexcept const<br>_Return a const reference to the sides vector._  |
|  std::size\_t | [**size**](#function-size) () noexcept const<br>_Return the number of stored ticks._  |
|  std::string\_view | [**symbol**](#function-symbol) () noexcept const<br>_Return the instrument symbol._  |
|  const std::vector&lt; uint64\_t &gt; & | [**timestamps**](#function-timestamps) () noexcept const<br>_Return a const reference to the timestamps vector._  |
|  bool | [**to\_csv**](#function-to_csv) (const std::string & path, std::size\_t chunk\_size, unsigned int decimal\_places=6) const<br>_Write tick data to a CSV file in chunks of_ `chunk_size` _rows._ |
|  std::string | [**to\_string**](#function-to_string) (const bool header=true, const unsigned int decimal\_places=6, std::size\_t start\_row=0U, std::size\_t end\_row=std::numeric\_limits&lt; std::size\_t &gt;::max()) const<br>_Render ticks as an ASCII CSV string._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::optional&lt; std::pair&lt; std::string, [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) &gt; &gt; | [**peek\_csv**](#function-peek_csv) (const std::string & path) <br>_Peek at the first data row of a CSV file without loading it._  |


























## Detailed Description


Each tick field is stored in its own contiguous vector so that operations on a single field (e.g., iterating all prices) from\_csv only the relevant data into cache and are SIMD-friendly.



* level == 0: trade (last traded price/quantity).
* level &gt;= 1: orderbook depth (1 = top of book, ...).




Call [**reserve()**](classmarketkernel_1_1MarketData.md#function-reserve) before the session starts to avoid reallocations.


Mode controls which fields are stored per [**append()**](classmarketkernel_1_1MarketData.md#function-append) call:
* ALL: all fields stored, including level (default).
* TRADE: level ignored and not stored; suited for trade-only feeds.
* LIQUIDITY: all fields stored; semantically scoped to orderbook updates.
* LEVEL: level ignored and not stored; suited for depth snapshots.




max\_level controls the maximum accepted level value. [**append()**](classmarketkernel_1_1MarketData.md#function-append) is a no-op when level &gt; max\_level. Default is 4.




**Template parameters:**


* `Num` Numeric type for price, quantity, and orders (e.g., `float`, `double`, or a user-defined fixed-point type). 




    
## Public Functions Documentation




### function MarketData [1/4]

_Default-constructs an empty container (symbol="", mode=ALL, max\_level=4)._ 
```C++
marketkernel::MarketData::MarketData () = default
```




<hr>



### function MarketData [2/4]

_Construct with a symbol name; mode defaults to ALL, max\_level to 4._ 
```C++
explicit marketkernel::MarketData::MarketData (
    const std::string & symbol
) 
```





**Parameters:**


* `symbol` Instrument symbol (e.g., "es", "nvda"). 




        

<hr>



### function MarketData [3/4]

_Construct with symbol and mode; max\_level defaults to 4 (0 for TRADE)._ 
```C++
marketkernel::MarketData::MarketData (
    const std::string & symbol,
    MarketDataMode mode
) 
```





**Parameters:**


* `symbol` Instrument symbol. 
* `mode` Storage mode controlling which fields are kept. 




        

<hr>



### function MarketData [4/4]

_Construct with symbol, mode, and explicit max\_level._ 
```C++
marketkernel::MarketData::MarketData (
    const std::string & symbol,
    MarketDataMode mode,
    uint8_t max_level
) 
```





**Parameters:**


* `symbol` Instrument symbol. 
* `mode` Storage mode. 
* `max_level` Maximum accepted orderbook level; ticks above this are dropped. 




        

<hr>



### function append 

_Append one tick; silently drops the tick when level &gt; max\_level._ 
```C++
void marketkernel::MarketData::append (
    uint64_t timestamp,
    Side side,
    uint8_t level,
    Num price,
    Num quantity,
    Num orders
) 
```





**Parameters:**


* `timestamp` Nanosecond epoch timestamp. 
* `side` Buy or sell side. 
* `level` Orderbook depth level (0 = trade, &gt;= 1 = depth). 
* `price` Tick price. 
* `quantity` Tick quantity. 
* `orders` Number of orders at this price level. 




        

<hr>



### function clear 

_Clear all tick vectors; symbol, mode, and max\_level are preserved._ 
```C++
void marketkernel::MarketData::clear () noexcept
```




<hr>



### function empty 

_Return true when no ticks have been stored._ 
```C++
inline bool marketkernel::MarketData::empty () noexcept const
```




<hr>



### function from\_csv 

_Load ticks from a CSV file produced by_ [_**to\_csv()**_](classmarketkernel_1_1MarketData.md#function-to_csv) _._
```C++
bool marketkernel::MarketData::from_csv (
    const std::string & path
) 
```



The header line is skipped automatically. Lines that fail to parse are skipped; a warning is printed to stderr with the 1-based line number. 

**Parameters:**


* `path` Path to the CSV file. 



**Returns:**

false only when the file cannot be opened. 





        

<hr>



### function levels 

_Return a const reference to the levels vector (empty in TRADE/LEVEL mode)._ 
```C++
inline const std::vector< uint8_t > & marketkernel::MarketData::levels () noexcept const
```




<hr>



### function load\_binary\_mmap 

_Load a binary snapshot via memory-mapped I/O for zero-copy ingestion._ 
```C++
bool marketkernel::MarketData::load_binary_mmap (
    const std::string & path
) 
```



Maps the file produced by [**save\_binary()**](classmarketkernel_1_1MarketData.md#function-save_binary) directly into the process address space using the OS memory-mapping facility. Where supported, the implementation requests huge / large pages to reduce TLB pressure when loading large datasets:



* **Linux** — POSIX `mmap(2)` with `MAP_PRIVATE`. After mapping, `madvise(2)` with `MADV_HUGEPAGE` requests Transparent Huge Page (THP) promotion (2 MB pages), and `MADV_SEQUENTIAL` enables kernel read-ahead. `MADV_HUGEPAGE` is compiled out on platforms that do not define it (macOS, older FreeBSD).
* **macOS / FreeBSD** — same `mmap(2)` path; `MADV_SEQUENTIAL` is applied; huge-page hints are omitted where unavailable.
* **Windows** — `CreateFileMapping` is first attempted with `SEC_LARGE_PAGES` (4 MB pages on x86-64) and `MapViewOfFile` with `FILE_MAP_LARGE_PAGES`. Both require `SeLockMemoryPrivilege`; if that privilege is not held the calls fail and the implementation retries transparently with standard 4 KB pages.




In all cases the kernel maps file data directly from the page cache, eliminating the kernel-buffer-to-user-buffer copy of `read()` / `fread()`.


On entry, the method validates:
* Magic bytes `'M'`,'K','M','D' at offset 0.
* Version byte equals `1`.
* `sizeof(Num)` byte matches the `Num` of this instantiation; attempting to load a `double` file into a `float` instance (or vice-versa) will be rejected here.
* File is large enough to hold all declared arrays.




On success the existing contents of the container are replaced by the loaded data; symbol\_, mode\_, and max\_level\_ are overwritten from the header. On failure the container state is unchanged.




**Note:**

The file is assumed to be in native byte order. Files produced on a big-endian host cannot be read on a little-endian host and vice-versa. 




**Note:**

Not thread-safe: do not call concurrently with any other method.




**Parameters:**


* `path` Path to a binary file produced by [**save\_binary()**](classmarketkernel_1_1MarketData.md#function-save_binary). 



**Returns:**

`true` on success. 




**Returns:**

`false` if the file cannot be opened, the header magic or version is wrong, `sizeof(Num)` mismatches, or the file is truncated. 





        

<hr>



### function max\_level 

_Return the maximum accepted orderbook level._ 
```C++
inline uint8_t marketkernel::MarketData::max_level () noexcept const
```




<hr>



### function mode 

_Return the storage mode._ 
```C++
inline MarketDataMode marketkernel::MarketData::mode () noexcept const
```




<hr>



### function orders 

_Return a const reference to the orders vector._ 
```C++
inline const std::vector< Num > & marketkernel::MarketData::orders () noexcept const
```




<hr>



### function prices 

_Return a const reference to the prices vector._ 
```C++
inline const std::vector< Num > & marketkernel::MarketData::prices () noexcept const
```




<hr>



### function quantities 

_Return a const reference to the quantities vector._ 
```C++
inline const std::vector< Num > & marketkernel::MarketData::quantities () noexcept const
```




<hr>



### function reserve 

_Pre-allocate capacity for_ `n` _ticks in all active field vectors._
```C++
void marketkernel::MarketData::reserve (
    std::size_t n
) 
```





**Parameters:**


* `n` Number of ticks to reserve capacity for. 




        

<hr>



### function save\_binary 

_Write a binary snapshot of all tick data to disk using block I/O._ 
```C++
bool marketkernel::MarketData::save_binary (
    const std::string & path
) const
```



Serialises the container to a compact binary format optimised for fast bulk reloads. The file begins with a fixed 32-byte header:



|Offset  |Size  |Field  |Notes  |
|-----|-----|-----|-----|
|0  |4  |magic  |`'M'`,'K','M','D'  |
|4  |1  |version  |`1`  |
|5  |1  |sizeof(Num)  |validated on load  |
|6  |1  |mode  |`MarketDataMode` cast to uint8\_t  |
|7  |1  |max\_level  |uint8\_t  |
|8  |8  |n\_ticks  |uint64\_t, native endian  |
|16  |8  |symbol\_len  |uint64\_t  |
|24  |1  |has\_levels  |`0` or `1`  |
|25  |7  |reserved  |zeros  |






Immediately after the header come `symbol_len` raw symbol bytes (no null terminator), followed by the six tick arrays written sequentially:
* `timestamps` — `uint64_t`[n\_ticks]
* `sides` — `Side`[n\_ticks]
* `levels` — `uint8_t`[n\_ticks] (omitted when `has_levels` == 0)
* `prices` — `Num`[n\_ticks]
* `quantities` — `Num`[n\_ticks]
* `orders` — `Num`[n\_ticks]




Each array is written in a single `std::fwrite` call, so I/O overhead is proportional to the number of arrays, not the number of ticks.


The resulting file can be reloaded with [**load\_binary\_mmap()**](classmarketkernel_1_1MarketData.md#function-load_binary_mmap).




**Note:**

The file is written in the native byte order of the host. Files are not portable across architectures with different endianness. 




**Note:**

Not thread-safe: do not call concurrently with [**append()**](classmarketkernel_1_1MarketData.md#function-append) or [**clear()**](classmarketkernel_1_1MarketData.md#function-clear).




**Parameters:**


* `path` Destination file path. An existing file is overwritten. 



**Returns:**

`true` on success. 




**Returns:**

`false` if the container is [**empty()**](classmarketkernel_1_1MarketData.md#function-empty), the file cannot be opened, or any write call fails. 





        

<hr>



### function sides 

_Return a const reference to the sides vector._ 
```C++
inline const std::vector< Side > & marketkernel::MarketData::sides () noexcept const
```




<hr>



### function size 

_Return the number of stored ticks._ 
```C++
inline std::size_t marketkernel::MarketData::size () noexcept const
```




<hr>



### function symbol 

_Return the instrument symbol._ 
```C++
inline std::string_view marketkernel::MarketData::symbol () noexcept const
```




<hr>



### function timestamps 

_Return a const reference to the timestamps vector._ 
```C++
inline const std::vector< uint64_t > & marketkernel::MarketData::timestamps () noexcept const
```




<hr>



### function to\_csv 

_Write tick data to a CSV file in chunks of_ `chunk_size` _rows._
```C++
bool marketkernel::MarketData::to_csv (
    const std::string & path,
    std::size_t chunk_size,
    unsigned int decimal_places=6
) const
```



The first chunk includes the header; subsequent chunks omit it. 

**Parameters:**


* `path` Destination file path. 
* `chunk_size` Number of rows per write chunk. 
* `decimal_places` Floating-point precision (default: 6). 



**Returns:**

false if the file cannot be opened, the container is [**empty()**](classmarketkernel_1_1MarketData.md#function-empty), or chunk\_size == 0. 





        

<hr>



### function to\_string 

_Render ticks as an ASCII CSV string._ 
```C++
std::string marketkernel::MarketData::to_string (
    const bool header=true,
    const unsigned int decimal_places=6,
    std::size_t start_row=0U,
    std::size_t end_row=std::numeric_limits< std::size_t >::max()
) const
```



Columns: symbol, mode, timestamp, side, level, price, quantity, orders. Floating-point columns use fixed notation with `decimal_places` digits (ignored for non-floating `Num`).


Row range is half-open [start\_row, end\_row). When `end_row` equals `std::numeric_limits<std::size_t>::max()` it is treated as [**size()**](classmarketkernel_1_1MarketData.md#function-size). Returns an empty string for invalid indices.




**Parameters:**


* `header` Include the header row when true (default: true). 
* `decimal_places` Floating-point precision (default: 6). 
* `start_row` First row index, inclusive (default: 0). 
* `end_row` Past-the-end row index (default: all rows). 



**Returns:**

CSV-formatted string, or empty string on invalid range. 





        

<hr>
## Public Static Functions Documentation




### function peek\_csv 

_Peek at the first data row of a CSV file without loading it._ 
```C++
static std::optional< std::pair< std::string, MarketDataMode > > marketkernel::MarketData::peek_csv (
    const std::string & path
) 
```





**Parameters:**


* `path` Path to the CSV file. 



**Returns:**

A pair {symbol, mode} from the first data row, or `std::nullopt` if the file cannot be opened, has no data rows, or the mode field is not a recognised value. 





        

<hr>## Friends Documentation





### friend operator&lt;&lt; 

_Serialize all ticks as CSV to an output stream._ 
```C++
template<typename N>
std::ostream & marketkernel::MarketData::operator<< (
    std::ostream & os,
    const MarketData < N > & md
) 
```




<hr>



### friend operator&gt;&gt; 

_Deserialize one CSV row from an input stream and append it._ 
```C++
template<typename N>
std::istream & marketkernel::MarketData::operator>> (
    std::istream & is,
    MarketData < N > & md
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `include/mk_market_data.hpp`

