# Project: MarketKernel

## Overview
- **Description:** A C++ base library for building trading indicators, strategies, and bots powered by neural networks, fuzzy logic, and genetic algorithms.
- **Language(s):** C++17
- **Build System:** CMake (minimum 3.20); tests via GoogleTest (fetched automatically via FetchContent)
- **Platform(s):** Cross-platform — Linux x86_64, Windows, macOS

## Repository Structure
- `include/` — Public headers: `mk_side.h` (Side enum), `mk_market_data.h` (MarketData class)
- `src/` — Implementation files: `mk_market_data.cpp`
- `tests/` — GoogleTest unit tests; test files mirror source names (e.g. `mk_market_data.cpp` → `mk_test_market_data.cpp`)

## Architecture & Key Decisions
- `MarketData<Num>` is a template class parametrized on the numeric type (`float`, `double`, or user-defined fixed-point/decimal). Explicit instantiations for `float` and `double` live in `mk_market_data.cpp`.
- Fields are stored in a Structure-of-Arrays (SoA) layout — each field (`timestamps`, `sides`, `levels`, `prices`, `quantities`, `orders`) is a separate contiguous vector for cache-efficient bulk calculations.
- `level == 0` means a trade; `level >= 1` means orderbook depth. Do not change this convention.
- Preferred compiler is **GCC**; other compilers (Clang, MSVC) are supported but GCC is the primary target.
- Compiler flags enforce `-Wall -Wextra -Wpedantic -Werror`; all warnings are errors.
- MarketData is used by symbol (`es`, `nvda`, `asml`)

## Coding Standards
- C++17 standard; no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- Primary compiler target is GCC; for hot SIMD routines, keep `[[gnu::always_inline]]` on explicit performance-critical functions such as `simd_sum<float>` and `simd_sum<double>`.
- No external runtime dependencies beyond the C++ standard library. GoogleTest is a test-only dependency.
- Each production module/source should have a mirrored test file in `tests/` named `mk_test_<module>.cpp` (for example, `src/mk_math_utils.cpp` -> `tests/mk_test_math_utils.cpp`).
- All new public API functions must be covered by unit tests in their mirrored module test file.

## Naming & File Conventions
- All source files (headers and implementations) must be prefixed with `mk_` (e.g. `mk_market_data.h`, `mk_market_data.cpp`).
- Test files follow the same prefix rule: `mk_test_<module>.cpp` (e.g. `mk_test_market_data.cpp`).
- Headers in `include/`, implementations in `src/`, tests in `tests/`

## AI & Computational Intelligence
- Neural networks, fuzzy logic systems, and genetic algorithms are first-class citizens in this project and will be used to implement indicators, strategies, and bots.
- AI components follow the same coding standards, file naming (`mk_` prefix), and test coverage requirements as all other modules.

## Out of Scope
- Do not add logging frameworks or external utility libraries without approval.
- Do not modify `CMakeLists.txt` unless the change is directly required by the task.
- Do not add code to `temp/`; it is scratch space only.
