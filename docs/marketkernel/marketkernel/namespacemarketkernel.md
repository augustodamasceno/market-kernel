

# Namespace marketkernel



[**Namespace List**](namespaces.md) **>** [**marketkernel**](namespacemarketkernel.md)




















## Classes

| Type | Name |
| ---: | :--- |
| class | [**MarketData**](classmarketkernel_1_1_market_data.md) &lt;typename Num&gt;<br>_Structure-of-Arrays market tick container for low-latency bulk calculations._  |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**MarketDataMode**](#enum-marketdatamode)  <br>_Controls which fields are stored per tick in_ [_**MarketData**_](classmarketkernel_1_1_market_data.md) _._ |
| enum uint8\_t | [**Side**](#enum-side)  <br>_Order side for a market tick: buy or sell._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  std::ostream & | [**operator&lt;&lt;**](#function-operator) (std::ostream & os, const [**MarketData**](classmarketkernel_1_1_market_data.md)&lt; Num &gt; & md) <br> |
|  std::istream & | [**operator&gt;&gt;**](#function-operator_1) (std::istream & is, [**MarketData**](classmarketkernel_1_1_market_data.md)&lt; Num &gt; & md) <br> |
|  Num | [**simd\_sum**](#function-simd_sum) (const std::vector&lt; Num &gt; & data) <br>_Sum all elements of a contiguous vector._  |
|  double | [**simd\_sum&lt; double &gt;**](#function-simd_sum-double) (const std::vector&lt; double &gt; & data) <br>_AVX-accelerated double specialization of_ [_**simd\_sum()**_](namespacemarketkernel.md#function-simd_sum) _._ |
|  float | [**simd\_sum&lt; float &gt;**](#function-simd_sum-float) (const std::vector&lt; float &gt; & data) <br>_AVX-accelerated float specialization of_ [_**simd\_sum()**_](namespacemarketkernel.md#function-simd_sum) _._ |
|  std::string\_view | [**to\_string**](#function-to_string) (const [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) mode) noexcept<br>_Convert a_ [_**MarketDataMode**_](namespacemarketkernel.md#enum-marketdatamode) _value to its lowercase string representation._ |
|  std::string\_view | [**to\_string**](#function-to_string) (const [**Side**](namespacemarketkernel.md#enum-side) side) noexcept<br>_Convert a_ [_**Side**_](namespacemarketkernel.md#enum-side) _value to its lowercase string representation._ |




























## Public Types Documentation




### enum MarketDataMode 

_Controls which fields are stored per tick in_ [_**MarketData**_](classmarketkernel_1_1_market_data.md) _._
```C++
enum marketkernel::MarketDataMode {
    ALL = 0,
    TRADE = 1,
    LIQUIDITY = 2,
    LEVEL = 3
};
```




* ALL: stores every field (timestamp, side, level, price, quantity, orders); use for full orderbook replay or multi-purpose datasets.
* TRADE: trade ticks only; level field is ignored and not stored; use when only last-traded-price/quantity data is needed.
* LIQUIDITY: stores all fields, semantically scoped to orderbook liquidity updates (adds, modifies, deletes).
* LEVEL: orderbook depth ticks; level field is ignored and not stored; use when per-level identity is irrelevant (e.g., mid-price, spread calculations) or to store a single level such as BBO. 




        

<hr>



### enum Side 

_Order side for a market tick: buy or sell._ 
```C++
enum marketkernel::Side {
    BUY = 0,
    SELL = 1
};
```




<hr>
## Public Functions Documentation




### function operator&lt;&lt; 

```C++
template<typename Num>
std::ostream & marketkernel::operator<< (
    std::ostream & os,
    const MarketData < Num > & md
) 
```




<hr>



### function operator&gt;&gt; 

```C++
template<typename Num>
std::istream & marketkernel::operator>> (
    std::istream & is,
    MarketData < Num > & md
) 
```




<hr>



### function simd\_sum 

_Sum all elements of a contiguous vector._ 
```C++
template<typename Num>
Num marketkernel::simd_sum (
    const std::vector< Num > & data
) 
```



Generic fallback uses std::accumulate (scalar, compiler auto-vectorizable). Explicit specializations for `float` and `double` use AVX vectorized reduction when compiled with AVX support, with a scalar remainder loop for any remaining elements.




**Template parameters:**


* `Num` Numeric element type; must support addition and zero-initialisation. 



**Parameters:**


* `data` Input vector. 



**Returns:**

Sum of all elements, or `Num{0}` if `data` is empty. 





        

<hr>



### function simd\_sum&lt; double &gt; 

_AVX-accelerated double specialization of_ [_**simd\_sum()**_](namespacemarketkernel.md#function-simd_sum) _._
```C++
template<>
inline double marketkernel::simd_sum< double > (
    const std::vector< double > & data
) 
```





**Parameters:**


* `data` Input vector of double values. 



**Returns:**

Sum of all elements, or 0.0 if `data` is empty. 





        

<hr>



### function simd\_sum&lt; float &gt; 

_AVX-accelerated float specialization of_ [_**simd\_sum()**_](namespacemarketkernel.md#function-simd_sum) _._
```C++
template<>
inline float marketkernel::simd_sum< float > (
    const std::vector< float > & data
) 
```





**Parameters:**


* `data` Input vector of float values. 



**Returns:**

Sum of all elements, or 0.0f if `data` is empty. 





        

<hr>



### function to\_string 

_Convert a_ [_**MarketDataMode**_](namespacemarketkernel.md#enum-marketdatamode) _value to its lowercase string representation._
```C++
inline std::string_view marketkernel::to_string (
    const MarketDataMode mode
) noexcept
```





**Parameters:**


* `mode` The [**MarketDataMode**](namespacemarketkernel.md#enum-marketdatamode) enum value to convert. 



**Returns:**

One of: "all", "trade", "liquidity", "level". 





        

<hr>



### function to\_string 

_Convert a_ [_**Side**_](namespacemarketkernel.md#enum-side) _value to its lowercase string representation._
```C++
inline std::string_view marketkernel::to_string (
    const Side side
) noexcept
```





**Parameters:**


* `side` The [**Side**](namespacemarketkernel.md#enum-side) enum value to convert. 



**Returns:**

"buy" for Side::BUY, "sell" for Side::SELL. 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `include/mk_market_data.hpp`

