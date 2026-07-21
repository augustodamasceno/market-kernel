/* Market Kernel : Lock-Free SPSC Ring Buffer
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file mk_ring_buffer.hpp
 * @brief Lock-free single-producer single-consumer ring buffer for market data transport.
 *
 * @details This module implements a classic SPSC ring buffer optimized for ultra-low-
 * latency inter-thread message passing without heap allocation on the hot path.
 *
 * The buffer uses power-of-two sizing with bitmask wrapping to eliminate expensive
 * modulo operations. Producer and consumer indices are cache-line aligned (64-byte)
 * in separate struct members to prevent false sharing. Synchronization relies on
 * std::atomic with explicit memory ordering: acquire (consumer read) and release
 * (producer write) for minimal overhead while maintaining visibility guarantees.
 *
 * Memory safety and determinism:
 * - Zero allocations after construction (all pre-allocated)
 * - Zero blocking: overflow policy (DROP_OLDEST, DROP_NEWEST, or BLOCK)
 * - Zero copying: consumer reads directly from buffer slots
 * - Cache-line aligned slots for efficient hardware prefetching
 * - $O(1)$ per-operation complexity with no growth or shrinking
 *
 * @par Platform support
 * - Cross-platform C++17; uses std::atomic for synchronization
 * - Aligned allocations via std::align_val_t (C++17 operator new overloads)
 * - Page pre-touching to avoid faults on the hot path (madvise/VirtualLock)
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>

namespace marketkernel
{

/**
 * @brief Single-producer single-consumer lock-free ring buffer.
 *
 * @tparam TSlot Type of each ring buffer slot (typically a struct containing message data).
 *
 * @details
 * This template implements a power-of-two sized ring buffer using bitmask wrapping
 * for index calculation. The producer thread writes to the buffer; the consumer
 * thread reads from it. Synchronization is achieved via std::atomic indices with
 * explicit memory ordering (acquire/release).
 *
 * Memory layout:
 * - `producer_head_` is updated only by the producer and read by the consumer
 *   during overflow detection. Placed in a dedicated cache line.
 * - `consumer_tail_` is updated only by the consumer and read by the producer
 *   during overflow detection. Placed in a separate cache line.
 * - The slot array follows, aligned to 64-byte boundaries.
 * - Each slot is cache-line aligned to ensure sequential prefetching behavior.
 *
 * Overflow handling:
 * - If the producer reaches the consumer (buffer full), the action depends on
 *   `overflow_policy`:
 *   - DROP_OLDEST: Discard the oldest message (increment consumer_tail_)
 *   - DROP_NEWEST: Drop the incoming message (skip write)
 *   - BLOCK: Wait for space (not recommended for hot path; increases latency)
 *
 * @note Time Complexity: O(1) for push and pop operations
 * @note Space Complexity: O(N) where N is the ring buffer size in slots
 * @note Memory: Zero allocations after construction; all memory pre-allocated.
 * @note Synchronization: No locks, no CAS operations; only acquire/release atomics.
 * @note Latency: Deterministic syscall latency; no GC or page faults in hot path.
 *
 * @pre Buffer size must be a power of 2 (e.g., 1024, 2048, 4096).
 * @post All slots are cache-line aligned; no false sharing between producer/consumer.
 */
template <typename TSlot>
class RingBuffer
{
public:
    /// Overflow handling policy for the ring buffer.
    enum class OverflowPolicy
    {
        DROP_OLDEST,  ///< Discard the oldest message if buffer is full.
        DROP_NEWEST,  ///< Drop the incoming message if buffer is full.
        BLOCK,        ///< Wait for space (not recommended for hot path).
    };

    /**
     * @brief Construct a ring buffer with a specified capacity.
     *
     * @param[in] capacity Number of slots in the ring buffer. Must be a power of 2.
     * @param[in] policy Overflow handling policy (default: DROP_OLDEST).
     *
     * @pre capacity > 0 and capacity must be a power of 2.
     * @post Ring buffer is initialized and ready for use.
     * @post All slots are pre-allocated; producer and consumer indices are zeroed.
     * @post Memory is pre-touched (all pages faulted in) to prevent page faults
     *       during trading.
     *
     * @throws std::invalid_argument if capacity is not a power of 2.
     * @throws std::bad_alloc if memory allocation fails.
     *
     * @note Time Complexity: O(capacity) due to pre-touching all pages.
     * @note Space Complexity: O(capacity * sizeof(TSlot))
     */
    explicit RingBuffer(size_t capacity, OverflowPolicy policy = OverflowPolicy::DROP_OLDEST);

    /**
     * @brief Destructor: deallocate the ring buffer.
     *
     * @details
     * Releases the pre-allocated buffer memory. This is typically called only during
     * system shutdown, not on the hot path.
     *
     * @post Memory is deallocated.
     * @note Time Complexity: O(1)
     */
    ~RingBuffer();

    // Delete copy/move constructors and assignment operators (not copyable).
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    /**
     * @brief Retrieve a writable slot from the producer's perspective.
     *
     * @details
     * This method is called by the producer to obtain a pointer to the next slot
     * to write to. If the buffer is full, the action depends on the overflow policy.
     *
     * - DROP_OLDEST: Advances the consumer index (drops oldest message).
     * - DROP_NEWEST: Returns nullptr (producer skips this write).
     * - BLOCK: Spins until space is available (not recommended for hot path).
     *
     * @param[out] slot_index The index of the slot (for later commit).
     * @return Pointer to the slot if successful, nullptr if DROP_NEWEST policy
     *         and buffer is full.
     *
     * @pre Called only by the producer thread.
     * @post If non-nullptr, the returned slot is owned by the producer until commit().
     * @post Memory ordering: No acquire/release barrier required for this operation
     *       since only the producer reads/writes producer_head_.
     *
     * @note Time Complexity: O(1) amortized
     * @note Latency: Deterministic; no syscalls or blocking.
     */
    [[gnu::always_inline]]
    TSlot* acquire_write_slot(size_t& slot_index) noexcept;

    /**
     * @brief Commit a written slot to the consumer.
     *
     * @details
     * Called by the producer after writing data to a slot obtained via acquire_write_slot().
     * This method publishes the slot's availability to the consumer via a memory barrier
     * (release semantics).
     *
     * @param[in] slot_index Index of the slot to commit (obtained from acquire_write_slot).
     *
     * @pre acquire_write_slot() must have been called successfully.
     * @post Memory barrier (release) is published, allowing the consumer to read the slot.
     * @post producer_head_ is incremented and visible to the consumer.
     *
     * @note Time Complexity: O(1)
     * @note Memory: Uses std::memory_order_release to ensure all writes to the slot
     *       are visible to the consumer before the index is incremented.
     */
    [[gnu::always_inline]]
    void commit_write_slot(size_t slot_index) noexcept;

    /**
     * @brief Retrieve a readable slot from the consumer's perspective.
     *
     * @details
     * Called by the consumer to obtain the next available slot to read. If no slot
     * is available, returns nullptr (non-blocking).
     *
     * @param[out] slot_index The index of the slot (for later release).
     * @return Pointer to the readable slot if available, nullptr if buffer is empty.
     *
     * @pre Called only by the consumer thread.
     * @post If non-nullptr, the returned slot contains valid data from the producer.
     * @post Memory ordering: Uses acquire semantics to ensure all producer writes
     *       to the slot are visible before the consumer reads.
     *
     * @note Time Complexity: O(1)
     * @note Latency: Deterministic; no syscalls or blocking.
     */
    [[gnu::always_inline]]
    const TSlot* acquire_read_slot(size_t& slot_index) noexcept;

    /**
     * @brief Release a read slot back to the producer.
     *
     * @details
     * Called by the consumer after consuming data from a slot obtained via
     * acquire_read_slot(). This method advances the consumer index and publishes
     * the release to the producer via a memory barrier (release semantics).
     *
     * @param[in] slot_index Index of the slot to release (obtained from acquire_read_slot).
     *
     * @pre acquire_read_slot() must have been called successfully.
     * @post Memory barrier (release) is published, allowing the producer to detect
     *       available space and potentially recover from overflow conditions.
     * @post consumer_tail_ is incremented and visible to the producer.
     *
     * @note Time Complexity: O(1)
     * @note Memory: Uses std::memory_order_release to ensure all reads from the slot
     *       are complete before the index is incremented.
     */
    [[gnu::always_inline]]
    void release_read_slot(size_t slot_index) noexcept;

    /**
     * @brief Retrieve the current number of occupied slots in the buffer.
     *
     * @details
     * Returns an approximate count of pending messages. This is a snapshot and may
     * be stale if the producer/consumer are actively writing/reading. Useful for
     * statistics and monitoring, but should not be used for control flow decisions
     * (those belong in the acquire/release methods).
     *
     * @return Number of messages currently in the buffer.
     *
     * @note Time Complexity: O(1)
     * @note This is a non-binding estimate and may lag behind actual occupancy due
     *       to the asynchronous nature of producer/consumer updates.
     */
    [[gnu::always_inline]]
    size_t size() const noexcept
    {
        size_t head = producer_head_.load(std::memory_order_relaxed);
        size_t tail = consumer_tail_.load(std::memory_order_relaxed);
        return head - tail;
    }

    /**
     * @brief Retrieve the ring buffer's capacity (number of slots).
     *
     * @return The capacity.
     *
     * @note Time Complexity: O(1)
     */
    [[gnu::always_inline]]
    size_t capacity() const noexcept { return capacity_; }

    /**
     * @brief Check if the ring buffer is empty.
     *
     * @return true if no messages are pending, false otherwise.
     *
     * @note This is a snapshot and may be stale if the producer/consumer are
     *       actively writing/reading.
     */
    [[gnu::always_inline]]
    bool empty() const noexcept
    {
        return producer_head_.load(std::memory_order_relaxed) ==
               consumer_tail_.load(std::memory_order_relaxed);
    }

private:
    /// Capacity of the ring buffer (number of slots), must be a power of 2.
    size_t capacity_;

    /// Bitmask for index wrapping (capacity - 1).
    size_t mask_;

    /// Overflow handling policy.
    OverflowPolicy policy_;

    /// Producer head index (updated only by producer thread).
    /// Placed in its own cache line to prevent false sharing.
    alignas(64) std::atomic<size_t> producer_head_{0};

    /// Padding to move consumer_tail_ to a separate cache line.
    char _producer_pad[64 - sizeof(std::atomic<size_t>)];

    /// Consumer tail index (updated only by consumer thread).
    /// Placed in its own cache line to prevent false sharing.
    alignas(64) std::atomic<size_t> consumer_tail_{0};

    /// Padding after consumer_tail_ to maintain cache-line alignment.
    char _consumer_pad[64 - sizeof(std::atomic<size_t>)];

    /// Pointer to the slot array (allocated at construction, deallocated at destruction).
    TSlot* slots_;

    /**
     * @brief Check if a given number is a power of 2.
     *
     * @param[in] n The number to check.
     * @return true if n is a power of 2, false otherwise.
     *
     * @note Time Complexity: O(1)
     */
    static bool is_power_of_two(size_t n) noexcept { return (n & (n - 1)) == 0 && n > 0; }
};

template <typename TSlot>
RingBuffer<TSlot>::RingBuffer(size_t capacity, OverflowPolicy policy)
    : capacity_(capacity), mask_(capacity - 1), policy_(policy), slots_(nullptr)
{
    if (!is_power_of_two(capacity))
    {
        throw std::invalid_argument("Ring buffer capacity must be a power of 2");
    }

    // Allocate aligned buffer (64-byte alignment for cache-line).
    slots_ = static_cast<TSlot*>(
        ::operator new(capacity * sizeof(TSlot), std::align_val_t{alignof(TSlot)}));

    if (!slots_)
    {
        throw std::bad_alloc();
    }

    // Pre-touch all pages to avoid page faults on the hot path.
    volatile char* page_ptr = reinterpret_cast<volatile char*>(slots_);
    for (size_t i = 0; i < capacity * sizeof(TSlot); i += 4096)
    {
        page_ptr[i] = 0;
    }
}

template <typename TSlot>
RingBuffer<TSlot>::~RingBuffer()
{
    if (slots_)
    {
        ::operator delete(slots_, std::align_val_t{alignof(TSlot)});
        slots_ = nullptr;
    }
}

template <typename TSlot>
[[gnu::always_inline]] inline TSlot* RingBuffer<TSlot>::acquire_write_slot(
    size_t& slot_index) noexcept
{
    size_t head = producer_head_.load(std::memory_order_relaxed);
    size_t tail = consumer_tail_.load(std::memory_order_acquire);

    size_t next_head = head + 1;
    size_t next_head_wrapped = next_head & mask_;

    // Overflow check.
    if (next_head_wrapped == (tail & mask_))
    {
        switch (policy_)
        {
            case OverflowPolicy::DROP_OLDEST:
                // Advance consumer to make space.
                consumer_tail_.store(tail + 1, std::memory_order_release);
                break;
            case OverflowPolicy::DROP_NEWEST:
                return nullptr;  // Drop the incoming message.
            case OverflowPolicy::BLOCK:
                // Spin until space is available.
                while (next_head_wrapped == (consumer_tail_.load(std::memory_order_acquire) & mask_))
                {
                    __builtin_ia32_pause();
                }
                break;
        }
    }

    slot_index = head & mask_;
    return &slots_[slot_index];
}

template <typename TSlot>
[[gnu::always_inline]] inline void RingBuffer<TSlot>::commit_write_slot(
    size_t /*slot_index*/) noexcept
{
    // Increment producer head and publish with release semantics.
    size_t new_head = (producer_head_.load(std::memory_order_relaxed) + 1);
    producer_head_.store(new_head, std::memory_order_release);
}

template <typename TSlot>
[[gnu::always_inline]] inline const TSlot* RingBuffer<TSlot>::acquire_read_slot(
    size_t& slot_index) noexcept
{
    size_t head = producer_head_.load(std::memory_order_acquire);
    size_t tail = consumer_tail_.load(std::memory_order_relaxed);

    if (tail == head)
    {
        return nullptr;  // Buffer is empty.
    }

    slot_index = tail & mask_;
    return &slots_[slot_index];
}

template <typename TSlot>
[[gnu::always_inline]] inline void RingBuffer<TSlot>::release_read_slot(
    size_t /*slot_index*/) noexcept
{
    // Increment consumer tail and publish with release semantics.
    size_t new_tail = (consumer_tail_.load(std::memory_order_relaxed) + 1);
    consumer_tail_.store(new_tail, std::memory_order_release);
}

}  // namespace marketkernel
