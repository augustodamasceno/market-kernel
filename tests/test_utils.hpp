/* Market Kernel : Test Utilities
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file test_utils.hpp
 * @brief Shared test utilities for all Market Kernel unit tests.
 *
 * @details Provides custom Google Test event listeners and other common
 * testing infrastructure used across the test suite.
 */

#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace marketkernel::test
{

/**
 * @class NanosecondTestListener
 * @brief Custom Google Test event listener reporting test execution time in nanoseconds.
 *
 * @details
 * Tracks high-resolution timing for each test using std::chrono::high_resolution_clock.
 * Produces output like: "TestName : 12,345,678 ns" for ultra-low-latency benchmarking.
 *
 * Usage in main():
 * @code
 *   int main(int argc, char** argv) {
 *       testing::InitGoogleTest(&argc, argv);
 *       auto* listener = new NanosecondTestListener();
 *       testing::TestEventListeners& listeners =
 *           testing::UnitTest::GetInstance()->listeners();
 *       listeners.Append(listener);
 *       return RUN_ALL_TESTS();
 *   }
 * @endcode
 */
class NanosecondTestListener : public testing::EmptyTestEventListener
{
private:
    using Clock = std::chrono::high_resolution_clock;
    std::map<std::string, Clock::time_point> test_starts_;

public:
    void OnTestStart(const testing::TestInfo& test_info) override
    {
        std::string test_name = 
            std::string(test_info.test_case_name()) + "." + test_info.name();
        test_starts_[test_name] = Clock::now();
    }

    void OnTestEnd(const testing::TestInfo& test_info) override
    {
        std::string test_name = 
            std::string(test_info.test_case_name()) + "." + test_info.name();
        
        auto it = test_starts_.find(test_name);
        if (it == test_starts_.end())
            return;

        auto elapsed = Clock::now() - it->second;
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

        // Format with automatic unit conversion
        std::string time_str;
        if (elapsed_ns >= 1000000)
        {
            // Display in milliseconds
            double ms = elapsed_ns / 1000000.0;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << ms << " ms";
            time_str = oss.str();
        }
        else if (elapsed_ns >= 1000)
        {
            // Display in microseconds
            double us = elapsed_ns / 1000.0;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << us << " us";
            time_str = oss.str();
        }
        else
        {
            // Display in nanoseconds
            time_str = std::to_string(elapsed_ns) + " ns";
        }

        // Format aligned with gtest output
        std::cout << "[ TIMING   ] " << test_name << " : " 
                  << std::setw(15) << time_str << std::endl;

        test_starts_.erase(it);
    }
};

}  // namespace marketkernel::test
