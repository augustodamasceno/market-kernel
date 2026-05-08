# Market Kernel : Tests CMake build
#
# Copyright (c) 2026, Augusto Damasceno. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause

# 1. Ensure FetchContent is managed correctly
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 2. Prevent GTest from overriding parent project settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# 3. Define test targets
add_executable(test_mk_market_data test_mk_market_data.cpp)
add_executable(test_mk_math test_mk_math.cpp)

# 4. Linkage (Prefer GTest::gmock if you need mocking later)
target_link_libraries(test_mk_market_data PRIVATE
    marketkernel_lib
    GTest::gtest_main
)
target_link_libraries(test_mk_math PRIVATE
    marketkernel_lib
    GTest::gtest_main
)

# 5. Compiler Warnings & Features
# High-quality code should enforce strict standards for trading and embedded
target_compile_features(test_mk_market_data PRIVATE cxx_std_17)
target_compile_options(test_mk_market_data PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_features(test_mk_math PRIVATE cxx_std_17)
target_compile_options(test_mk_math PRIVATE -Wall -Wextra -Wpedantic -Werror)

# 6. Test Discovery
include(GoogleTest)
gtest_discover_tests(test_mk_market_data)
gtest_discover_tests(test_mk_math)