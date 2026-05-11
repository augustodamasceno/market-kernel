# Language Rules: C / C++

## Documentation Standards (Doxygen)

All code must be strictly documented using Doxygen standard formatting (`/** ... */` or `///`) to ensure full transparency of interface contracts and system behavior. 

### Core Requirements

* **Interface Contracts:** Every class, function, and method must define its exact purpose (`@brief`), structural details (`@details`), parameter directions (`@param[in]`, `@param[out]`, `@param[in,out]`), and return values (`@return`).
* **Template Semantics:** All template parameters must be explicitly documented using `@tparam`, detailing required type traits, constraints, or expected concepts.
* **Performance Guarantees:** * Document algorithmic time and space complexity using Big O notation (e.g., $O(1)$, $O(N \log N)$).
    * Explicitly declare memory allocation behavior (e.g., "Zero heap allocations in the hot path").
    * State hardware utilization intent (e.g., SIMD vectorization, cache line alignment requirements).
* **Safety & Execution:** Clearly define preconditions (`@pre`), postconditions (`@post`), thread-safety characteristics, and exception safety guarantees (validating `noexcept` specifications or documenting exceptions using `@throws`).
* **Architectural Scope:** All header (`.hpp`) and source (`.cpp`) files must contain a `@file` block with licensing, copyright, and a high-level architectural description of the translation unit.

### Example Template

Use the following snippet as a baseline for documenting high-performance functions:

```cpp
/**
 * @brief Computes the SIMD-accelerated sum of an aligned vector.
 *
 * @details Utilizes AVX intrinsics to process elements in 32-byte chunks.
 * The remainder loop handles arrays that are not a multiple of the vector width.
 *
 * @pre The input vector must not be empty.
 * @post Returns the mathematically precise sum of all elements.
 *
 * @tparam Num A standard layout floating-point type (float or double).
 *
 * @param[in] data Read-only reference to the contiguous memory vector.
 * @return The calculated sum.
 *
 * @note Time Complexity: $O(N)$
 * @note Space Complexity: $O(1)$
 * @note Memory: Guarantees zero heap allocations.
 */
template<typename Num>
[[gnu::always_inline]]
inline Num simd_sum(const std::vector<Num>& data) noexcept;
```

## Code Style
- **Brace Style:** Allman Style (Braces always on a new line).
- **Switch Statements:** Align `case` labels vertically with the `switch` keyword.
- **Include Guards:** Use `#pragma once`.

## Naming Conventions
| Entity | Case / Style | Example |
| :--- | :--- | :--- |
| **Files** | snake_case (lowercase) | `ecc_curve_defs.h` |
| **Private Members** | camelCase + trailing `_` | `value_` |
| **Macros** | UPPER_SNAKE_CASE | `CACHE_SIZE_DEFAULT` |
| **Variables** | snake_case | `item_width` |
| **Functions** | snake_case | `string_size()` |
| **Methods** | snake_case | `reset_geometry()` |
| **Constants** | PascalCase | `Fps` |
| **Structs** | PascalCase | `MovingCorrelation` |
| **Enums** | UPPER_SNAKE_CASE (Elements) | `BMP`, `JPEG` |
| **Classes** | PascalCase | `URLSecurityManager` |
| **Namespaces** | lowercase (start w/ project) | `projectplot` |

## Snippet: Switch Alignment
```c
switch (suffix)
{
case 'G': // Aligned with switch
    mem <<= 30;
    break;
default:
    break;
}
```
