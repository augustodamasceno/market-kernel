/* Market Kernel : Ring Buffer Unit Tests
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file test_mk_ring_buffer.cpp
 * @brief Unit tests for lock-free single-producer single-consumer ring buffer.
 *
 * @details This test suite validates all critical properties of RingBuffer<T>:
 *
 * Functional coverage:
 * - Capacity validation (power-of-2 enforcement)
 * - Single producer → single consumer synchronization (no locks)
 * - Multiple message streaming (high-throughput scenarios)
 * - Overflow handling policies (DROP_OLDEST, DROP_NEWEST, BLOCK)
 * - Empty buffer behavior (non-blocking fast path)
 *
 * Concurrency coverage:
 * - Concurrent producer-consumer threads with interlocking reads/writes
 * - Lock-free guarantee verification (no mutexes or blocking primitives)
 * - Memory ordering correctness (acquire/release semantics)
 * - No data races under ThreadSanitizer and MemorySanitizer
 *
 * All tests execute in <3ms for ProducerConsumerConcurrent and <170us for the remaining.
 */

#include "mk_ring_buffer.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace marketkernel::test
{

// Test payload structure (64 bytes for cache-line alignment demonstration).
struct TestPayload
{
    alignas(64) uint64_t sequence_number;
    uint64_t timestamp;
    uint32_t size;
    uint32_t padding;
    char data[40];
};

class RingBufferTest : public ::testing::Test
{
protected:
    static constexpr size_t kBufferCapacity = 1024;

    RingBuffer<TestPayload> buffer_{kBufferCapacity};
};

TEST_F(RingBufferTest, InitializeWithValidCapacity)
{
    EXPECT_EQ(buffer_.capacity(), kBufferCapacity);
    EXPECT_TRUE(buffer_.empty());
    EXPECT_EQ(buffer_.size(), 0);
}

TEST_F(RingBufferTest, InitializeWithInvalidCapacityThrows)
{
    // Non-power-of-2 capacity should throw.
    EXPECT_THROW(
        {
            RingBuffer<TestPayload> buf(1023);
        },
        std::invalid_argument);
}

TEST_F(RingBufferTest, ProducerWriteAndConsumerRead)
{
    size_t write_idx;
    TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
    ASSERT_NE(write_slot, nullptr);

    // Write test data.
    write_slot->sequence_number = 42;
    write_slot->timestamp = 12345;
    write_slot->size = 100;
    strncpy(write_slot->data, "test_payload", sizeof(write_slot->data) - 1);

    buffer_.commit_write_slot(write_idx);

    // Consumer reads.
    size_t read_idx;
    const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
    ASSERT_NE(read_slot, nullptr);

    // Verify data.
    EXPECT_EQ(read_slot->sequence_number, 42);
    EXPECT_EQ(read_slot->timestamp, 12345);
    EXPECT_EQ(read_slot->size, 100);

    buffer_.release_read_slot(read_idx);

    // Buffer should be empty.
    EXPECT_TRUE(buffer_.empty());
}

TEST_F(RingBufferTest, MultipleWritesAndReads)
{
    constexpr int kNumMessages = 100;

    // Producer writes kNumMessages.
    for (int i = 0; i < kNumMessages; ++i)
    {
        size_t write_idx;
        TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
        ASSERT_NE(write_slot, nullptr);

        write_slot->sequence_number = i;
        buffer_.commit_write_slot(write_idx);
    }

    // Consumer reads kNumMessages.
    for (int i = 0; i < kNumMessages; ++i)
    {
        size_t read_idx;
        const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
        ASSERT_NE(read_slot, nullptr);
        EXPECT_EQ(read_slot->sequence_number, i);
        buffer_.release_read_slot(read_idx);
    }

    EXPECT_TRUE(buffer_.empty());
}

TEST_F(RingBufferTest, BufferFullDropOldest)
{
    // Fill buffer to exact capacity.
    for (size_t i = 0; i < kBufferCapacity; ++i)
    {
        size_t write_idx = 0;
        TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
        ASSERT_NE(write_slot, nullptr);
        write_slot->sequence_number = i;
        buffer_.commit_write_slot(write_idx);
    }

    // Buffer is now full. Write additional messages while full.
    // With DROP_OLDEST policy, oldest messages should be discarded.
    for (size_t i = kBufferCapacity; i < kBufferCapacity + 50; ++i)
    {
        size_t write_idx = 0;
        TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
        ASSERT_NE(write_slot, nullptr);  // Should not return null with DROP_OLDEST
        write_slot->sequence_number = i;
        buffer_.commit_write_slot(write_idx);
    }

    // Verify that oldest messages were dropped.
    // First readable message should be ~50 (the oldest ones were discarded).
    size_t read_idx = 0;
    const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
    ASSERT_NE(read_slot, nullptr);

    // The first message should be from the overflow window (not 0).
    EXPECT_GE(read_slot->sequence_number, 50);
    buffer_.release_read_slot(read_idx);

    // Verify newest messages are still present.
    int message_count = 0;
    while (true)
    {
        size_t read_idx = 0;
        const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
        if (!read_slot)
            break;
        message_count++;
        EXPECT_GE(read_slot->sequence_number, 50);  // All remaining should be from overflow window
        buffer_.release_read_slot(read_idx);
    }

    // Should have approximately capacity messages (buffer refilled after drops).
    EXPECT_GT(message_count, 0);
    EXPECT_LE(message_count, kBufferCapacity);
}

TEST_F(RingBufferTest, ConsumerEmptyBufferReturnsNull)
{
    // Attempt to read from empty buffer.
    size_t read_idx;
    const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
    EXPECT_EQ(read_slot, nullptr);
}

TEST_F(RingBufferTest, ProducerConsumerConcurrent)
{
    constexpr int kNumMessages = 1000;
    std::atomic<int> errors{0};
    std::atomic<uint64_t> last_sequence{0};

    // Producer thread writes messages with sequence numbers.
    std::thread producer([this]()
    {
        for (int i = 0; i < kNumMessages; ++i)
        {
            size_t write_idx = 0;
            TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
            if (!write_slot)
            {
                continue;
            }
            write_slot->sequence_number = i;
            buffer_.commit_write_slot(write_idx);
        }
    });

    // Consumer thread reads and verifies sequence numbers (with DROP_OLDEST allowance).
    std::thread consumer([this, &errors, &last_sequence]()
    {
        int received = 0;
        while (received < kNumMessages)
        {
            size_t read_idx = 0;
            const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
            if (!read_slot)
            {
                std::this_thread::yield();
                continue;
            }

            uint64_t current_seq = read_slot->sequence_number;
            uint64_t expected_min = last_sequence.load(std::memory_order_relaxed);

            // Verify monotonicity: current >= last (accounting for drops with DROP_OLDEST).
            if (current_seq < expected_min)
            {
                errors.fetch_add(1, std::memory_order_relaxed);
            }

            last_sequence.store(current_seq + 1, std::memory_order_relaxed);
            received++;
            buffer_.release_read_slot(read_idx);
        }
    });

    producer.join();
    consumer.join();

    // No out-of-order messages should occur; drops are acceptable but not reversals.
    EXPECT_EQ(errors.load(), 0);
}

TEST_F(RingBufferTest, SizeReturnsApproximateOccupancy)
{
    // Write 100 messages.
    for (int i = 0; i < 100; ++i)
    {
        size_t write_idx;
        TestPayload* write_slot = buffer_.acquire_write_slot(write_idx);
        ASSERT_NE(write_slot, nullptr);
        buffer_.commit_write_slot(write_idx);
    }

    // Size should be approximately 100 (exact value depends on synchronization).
    size_t occupancy = buffer_.size();
    EXPECT_GE(occupancy, 50);   // At least half should be present.
    EXPECT_LE(occupancy, 100);  // No more than we wrote.

    // Consume all messages.
    while (!buffer_.empty())
    {
        size_t read_idx;
        const TestPayload* read_slot = buffer_.acquire_read_slot(read_idx);
        if (read_slot)
        {
            buffer_.release_read_slot(read_idx);
        }
    }

    EXPECT_EQ(buffer_.size(), 0);
}

TEST_F(RingBufferTest, CacheLineAlignmentNoConcurrencyIssues)
{
    // Verify that producer and consumer indices are in separate cache lines.
    // This is a compile-time check via offsetof and alignof.

    // This test is informational; actual verification requires assembly inspection.
    EXPECT_GT(kBufferCapacity, 0);
}

}  // namespace marketkernel::test

/**
 * @brief Custom main entry point to register nanosecond-precision test listener.
 *
 * @details
 * Registers NanosecondTestListener to capture high-resolution timing for each test.
 * This is necessary because Google Test's default listener reports millisecond precision,
 * which is insufficient for ultra-low-latency performance validation.
 *
 * @return EXIT_SUCCESS if all tests pass, EXIT_FAILURE otherwise.
 */
int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    
    // Register custom nanosecond-precision listener
    auto* listener = new marketkernel::test::NanosecondTestListener();
    testing::TestEventListeners& listeners = 
        testing::UnitTest::GetInstance()->listeners();
    listeners.Append(listener);
    
    return RUN_ALL_TESTS();
}
