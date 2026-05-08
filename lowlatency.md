# Ultra-Low Latency C++ / Quant Trading — LLM Instruction Set

## Scope and Enforcement

These directives apply to all code on the **critical trading path**: market data ingestion, order book maintenance, signal computation, order generation, and order routing. Code off the hot path (risk checks batched at 1Hz, admin interfaces, logging pipelines) may relax individual rules where noted. When in doubt, assume hot path.

Do not default to standard software engineering idioms if they violate these rules. Correctness at nanosecond scale and correctness at human scale are different things.

---

## 1. Memory Management

**No heap allocation on the hot path.**
Reject `new`, `delete`, `malloc`, `free`, `std::make_shared`, `std::make_unique` (when it implies heap), and any container resize (`push_back` beyond reserved capacity, `insert` into unordered maps, etc.). All data structures must be pre-allocated and pre-sized at system startup using arena allocators, object pools, or stack allocation.

**Arena and pool allocators.**
Use `std::pmr::monotonic_buffer_resource` or a custom arena for objects with the same lifetime. Use fixed-capacity object pools for order objects, message structs, and market data snapshots. Pool slots are claimed and released without any OS call.

**Huge pages.**
Allocate large ring buffers, order books, and shared memory regions using 2 MB or 1 GB huge pages (`mmap` with `MAP_HUGETLB`, or `libhugetlbfs`) to eliminate TLB misses from fine-grained page table walks.

**Stack allocation for temporaries.**
Prefer `std::array`, `alloca`-equivalent patterns, or fixed-size structs on the stack for transient computation. Never use `std::string` on the hot path; use `char[]` arrays or `std::string_view` for read-only observation.

**Memory-mapped files.**
Prefer `mmap` over `read`/`write` syscalls for reference data (symbology tables, config). Map once at startup; treat as read-only on the hot path.

---

## 2. Cache and Memory Hierarchy

**Cache line size is 64 bytes; treat it as the atomic unit of memory.**
Pad and align hot structs to 64-byte boundaries with `alignas(64)`. Never share a cache line between two threads that write independently (false sharing). Use `[[no_unique_address]]` and explicit padding fields to control layout.

**False sharing prevention.**
Separate producer and consumer fields of a queue into distinct cache lines. Place a `char _pad[64]` between fields updated by different threads. Verify layout with `static_assert(offsetof(T, field) % 64 == 0, ...)`.

**Data locality.**
Use structure-of-arrays (SoA) over array-of-structures (AoS) when processing a single field across many instruments (e.g., scanning bid prices for 500 symbols). Hot fields first in structs; cold fields (audit, logging metadata) in a separate struct referenced by pointer and only touched off the hot path.

**Prefetching.**
Use `__builtin_prefetch(ptr, 0, 3)` (read, high temporal locality) to pull the next iteration's data into L1 while processing the current one. In order book traversal loops, prefetch 2–4 iterations ahead. Do not over-prefetch — excess prefetches pollute the cache.

**Write combining.**
Group sequential writes to the same cache line. Avoid interleaving writes to non-adjacent cache lines in tight loops. Use non-temporal stores (`_mm_stream_si64`) only for large sequential writes that will not be re-read, such as logging ring buffers.

**NUMA awareness.**
Pin each thread to a NUMA node and allocate its working memory from the same node using `numa_alloc_onnode` or `mbind`. Cross-NUMA memory accesses add ~100 ns. Verify placement with `numactl --hardware` and `numastat`.

---

## 3. CPU and Core Affinity

**CPU isolation.**
Isolate hot-path cores with `isolcpus=`, `nohz_full=`, and `rcu_nocbs=` kernel boot parameters. Isolated cores receive no OS scheduling, RCU callbacks, or timer ticks. Confirm with `/sys/devices/system/cpu/cpu<N>/`.

**Thread pinning.**
Pin every hot-path thread to a dedicated isolated core using `pthread_setaffinity_np` or `sched_setaffinity`. Never let two competing threads share a physical core (hyperthreading sibling). Disable HT on hot-path cores or leave HT siblings idle.

**CPU frequency.**
Set the governor to `performance` and pin frequency at the maximum non-turbo value. Turbo boost introduces frequency jitter that destroys latency percentiles. Use `cpupower frequency-set` or write directly to `/sys/devices/system/cpu/cpu<N>/cpufreq/`.

**Real-time scheduling.**
Use `SCHED_FIFO` with priority 99 for the hot-path thread (`pthread_setschedparam`). This prevents the OS from preempting it for any lower-priority task.

**IRQ affinity.**
Move all NIC interrupt handlers off hot-path cores. Write target CPUs to `/proc/irq/<N>/smp_affinity_list`. For kernel-bypass NICs (DPDK, RDMA), there are no IRQs to worry about on the application side.

---

## 4. Branching and Control Flow

**Predictable branching only.**
The branch predictor has ~4-cycle misprediction penalty on modern CPUs, rising to 15–20 cycles for deeply pipelined paths. Flag any branch in the hot path whose outcome depends on runtime market data as high-risk.

**Branchless arithmetic.**
Replace `if (a > b) result = a; else result = b;` with `result = a + ((b - a) & -(b > a));` or use `std::max` when the compiler can emit `cmov`. Verify with `-S` output or `objdump`.

**Compiler hints.**
Use `[[likely]]` / `[[unlikely]]` (C++20) or `__builtin_expect` on branches that are structurally predictable (error paths, initialization guards). Do not annotate branches where both outcomes are equally likely — the annotation costs nothing when correct but misleads the predictor when wrong.

**Lookup tables.**
Precompute any function with bounded integer input into a `constexpr` array. Side lookups (tick size by price level, fee tier by volume band) must be arrays, not `switch` or `std::map`.

**Avoid virtual dispatch.**
No `virtual` functions on the hot path. Use CRTP (curiously recurring template pattern) or `std::variant` with `std::visit` for compile-time polymorphism. If runtime polymorphism is unavoidable, use a function pointer table and verify it resolves to a single branch, not a double indirect.

**No exceptions on the hot path.**
Disable exception propagation through hot-path code. Use `noexcept` on all hot-path functions. Return error codes or `std::expected`; never throw. Exceptions are zero-cost when not thrown but their presence prevents certain compiler optimizations.

**No RTTI on the hot path.**
Avoid `dynamic_cast` and `typeid`. Compile with `-fno-rtti` if the hot-path translation units can be isolated. Use tagged unions or type-safe discriminated unions instead.

---

## 5. Compiler and Code Generation

**Optimization flags.**
Compile hot-path translation units with `-O3 -march=native -mtune=native -funroll-loops -ffast-math` (only if IEEE 754 strict compliance is not required). Use `-flto` (link-time optimization) for whole-program inlining across translation unit boundaries.

**Force inlining.**
Mark all hot-path functions `__attribute__((always_inline))` or `[[gnu::always_inline]]`. Do not rely on the compiler's heuristics — at `-O3` a 50-instruction function may still not be inlined if the call site is a dispatch loop. Verify in assembly output.

**`__attribute__((hot))` and `__attribute__((cold))`.**
Annotate hot-path functions with `__attribute__((hot))` so the compiler places them in a contiguous `.text.hot` section, improving instruction cache locality. Annotate error and logging paths `__attribute__((cold))` so they are placed far from hot code.

**`constexpr` and compile-time computation.**
Push all constant computations to compile time. Fee schedules, tick-size tables, protocol field offsets, and bitmasks must be `constexpr`. Use `if constexpr` in templates to eliminate dead branches at compile time.

**SIMD vectorization.**
Use SSE4.2 / AVX2 intrinsics for bulk operations: scanning multiple price levels simultaneously, computing multiple VWAP numerators in one pass, checksumming packets. Prefer auto-vectorization with `#pragma GCC ivdep` and aligned allocations before dropping to manual intrinsics. Manual intrinsics are fragile; prefer them only after measuring.

**`__restrict__`.**
Annotate pointer parameters that are guaranteed non-aliased with `__restrict__` to allow the compiler to vectorize and reorder loads/stores across them.

---

## 6. Concurrency and Synchronization

**No kernel-mode locking on the hot path.**
`std::mutex`, `std::shared_mutex`, `pthread_mutex_t`, and any syscall-based primitive (`futex`, `sem_wait`) are forbidden. Any blocking call on the hot path is a latency cliff.

**`std::atomic` with explicit memory ordering.**
Use the weakest correct memory order:
- `memory_order_relaxed` for counters where only the latest value matters and ordering relative to other variables is not required.
- `memory_order_acquire` / `memory_order_release` for producer-consumer handoff (one load, one store).
- `memory_order_seq_cst` only when a total order across multiple threads is provably necessary — it emits a full memory fence (`mfence` on x86) which costs ~30 cycles.
- `memory_order_acq_rel` for compare-exchange on a shared state variable.

Never use `memory_order_seq_cst` by default. Audit every atomic and justify its ordering.

**SPSC queues.**
Single-producer single-consumer lock-free ring buffers are the canonical inter-thread transport. Use a power-of-two capacity with a bitmask index, a head index owned by the producer, and a tail index owned by the consumer — each in its own cache line. No atomic CAS required; a single `store(release)` / `load(acquire)` pair suffices.

**MPSC / MPMC queues.**
For multi-producer scenarios, use a CAS-based MPSC queue (e.g., Dmitry Vyukov's intrusive MPSC queue) or a bounded MPMC queue with per-slot sequence numbers. Avoid third-party queue libraries that hide memory allocations.

**Busy spin, not sleep.**
Hot-path threads must spin (`while (!queue.try_pop()) { _mm_pause(); }`) rather than yield or sleep. `_mm_pause()` (x86 `PAUSE` instruction) reduces power consumption and avoids pipeline stalls when spinning on a memory location.

**Seqlocks.**
For read-heavy data (top-of-book snapshot read by many strategies, written by one feed handler), use a seqlock: an `atomic<uint64_t>` sequence counter incremented before and after each write. Readers spin until the counter is even and unchanged across their read. Zero writer contention; readers retry only on a concurrent write.

---

## 7. Network and I/O Stack

**Kernel bypass.**
For market data and order routing, bypass the Linux network stack entirely. Use DPDK (Data Plane Development Kit) for polling-mode Ethernet, or RDMA (Remote Direct Memory Access / Infiniband / RoCE) for lowest-latency exchange connectivity. Kernel bypass eliminates context switches, interrupt coalescing delay, and socket buffer copies.

**Onload / solarflare ef_vi.**
When kernel bypass is required but DPDK is not available, use the `ef_vi` API (OpenOnload / SolarCapture) for wire-to-application latency under 1 µs.

**UDP multicast for market data.**
Prefer UDP multicast feeds (CME MDP 3.0, OPRA, ICE iMpact) over TCP. Design the feed handler to process packets in-place from the NIC ring buffer with zero copies. Detect gaps by sequence number and trigger a retransmit request off the hot path.

**TCP_NODELAY and SO_BUSY_POLL.**
For TCP order entry (FIX, OUCH, ITCH): set `TCP_NODELAY` to disable Nagle, `SO_BUSY_POLL` with a spin interval to avoid IRQ latency, and `SO_SNDBUF` / `SO_RCVBUF` sized to avoid buffer bloat. Consider `SO_TXTIME` with hardware timestamping for precise send scheduling.

**Zero-copy message parsing.**
Parse binary protocol messages (SBE, ITCH, OUCH, CME iLink) directly from the NIC buffer or shared memory region without copying into an intermediate struct. Use `reinterpret_cast` on aligned buffers or `std::bit_cast` (C++20). Define protocol structs with `__attribute__((packed))` only when the wire layout is non-aligned, and add explicit alignment padding for fields that will be read in hot loops.

**Hardware timestamping.**
Use NIC hardware timestamps (via `SO_TIMESTAMPING` with `SOF_TIMESTAMPING_RX_HARDWARE`) for market data arrival time. Software timestamps (`gettimeofday`, `clock_gettime`) have ~100 ns jitter from interrupt scheduling. Hardware timestamps have sub-10 ns accuracy.

---

## 8. Timing and Clocks

**TSC for hot-path timing.**
Use `RDTSC` / `RDTSCP` for high-resolution elapsed time on the hot path. `RDTSCP` serializes the instruction stream and reads the processor ID, ensuring the timestamp is taken after all preceding instructions. Calibrate TSC frequency at startup against `clock_gettime(CLOCK_REALTIME)`.

```cpp
inline uint64_t rdtsc() noexcept {
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi) :: "ecx", "memory");
    return (static_cast<uint64_t>(hi) << 32) | lo;
}
```

**TSC invariance.**
Require `constant_tsc` and `nonstop_tsc` CPU flags (`grep -m1 flags /proc/cpuinfo`). Without these, TSC frequency changes with CPU power states. Set the CPU to a fixed frequency (no turbo) before trusting TSC as a wall-clock source.

**`clock_gettime(CLOCK_MONOTONIC)` off the hot path.**
For timestamps that must survive across warm-up and logging, use `CLOCK_MONOTONIC`. Never use `CLOCK_REALTIME` for interval measurement — it can jump due to NTP adjustments.

**PTP / IEEE 1588.**
For cross-machine latency measurement and synchronized order timestamps, use PTP hardware clocks (`/dev/ptpN`) disciplined by a grandmaster. Software PTP (`linuxptp`) achieves ~100 ns accuracy on local Ethernet; hardware PTP with a capable NIC achieves sub-10 ns.

---

## 9. Trading Data Structures

**Order book.**
Use a price-level array indexed by price offset from a reference price (e.g., `levels[price - min_price]`). Avoid `std::map` or any tree structure — O(log n) traversal and pointer chasing are unacceptable. Fixed-size arrays sized to the maximum expected spread (e.g., 10,000 ticks) pre-allocated at startup. Store bid and ask sides as two separate contiguous arrays.

**Ring buffers.**
Use power-of-two sized ring buffers with bitmask wrapping (`index & (size - 1)`) for all inter-thread queues, logging, and sequence number tracking. Pre-touch all pages at startup with `memset` to avoid page faults during trading.

**Struct layout for market data.**
Put the most-accessed fields (price, quantity, side, timestamp) first in order structs. Ensure the entire hot portion fits in one or two cache lines. Move instrument ID, venue, account, and flags to a second struct only accessed for routing.

**Flat hash maps.**
When a hash map is required off the critical path, use an open-addressing flat hash map (e.g., `absl::flat_hash_map`, `ankerl::unordered_dense`) rather than `std::unordered_map` (chained, heap-allocated buckets). Pre-reserve to the expected maximum load factor to prevent rehashing.

**Symbol tables.**
Encode instrument identifiers as integer IDs (16 or 32-bit) as early as possible in the ingestion pipeline. All hot-path code uses integer IDs; string lookups occur only at message boundaries, off the hot path.

---

## 10. System-Level Configuration

The following OS and kernel settings must be applied before the trading system starts. Treat missing settings as bugs.

| Setting | Command / File | Value |
|---|---|---|
| CPU isolation | `/etc/default/grub` `GRUB_CMDLINE_LINUX` | `isolcpus=2-7 nohz_full=2-7 rcu_nocbs=2-7` |
| CPU frequency governor | `cpupower frequency-set -g performance` | `performance` |
| Turbo boost off | `/sys/devices/system/cpu/intel_pstate/no_turbo` | `1` |
| Transparent huge pages | `/sys/kernel/mm/transparent_hugepage/enabled` | `never` (use explicit huge pages) |
| IRQ balance off | `systemctl stop irqbalance` | disabled |
| NIC IRQs off hot cores | `/proc/irq/<N>/smp_affinity_list` | non-isolated CPUs |
| NUMA balancing off | `/proc/sys/kernel/numa_balancing` | `0` |
| Swap off | `swapoff -a` | disabled |
| Kernel preemption | Kernel config `CONFIG_PREEMPT_RT` | real-time patch (production) |

---

## 11. Measurement and Profiling

**Measure before optimizing.**
Every proposed optimization must be validated by measurement. Intuition about latency is frequently wrong below 1 µs. Use latency histograms (50th, 99th, 99.9th, 99.99th percentile), not averages. A 200 ns mean with 50 µs P99.99 is worse than a 500 ns mean with 2 µs P99.99 for most trading strategies.

**Benchmarking tooling.**
Use Google Benchmark with `benchmark::DoNotOptimize` and `benchmark::ClobberMemory` to prevent the compiler from eliminating benchmark loops. Report cycles per operation using `RDTSC` brackets, not wall-clock time, to decouple from CPU frequency variation.

**Profiling tools.**
- `perf stat -e cycles,instructions,cache-misses,branch-misses` for a first-pass breakdown.
- `perf record -g -F 999` + `perf report` for hot function identification.
- Intel VTune Profiler (`vtune -collect uarch-exploration`) for microarchitectural analysis (front-end bound, back-end bound, bad speculation).
- `pmu-tools / toplev` for top-down methodology analysis without VTune.

**Avoid benchmarking cold caches.**
Warm up all data structures and code paths before recording latency. The first 1,000 iterations through a loop are not representative; discard them. JIT effects (branch predictor training, instruction cache fill) require hundreds of iterations to stabilize.

**Latency spikes.**
Median latency is not the trading system's latency — tail latency is. Instrument every stage gate with a `RDTSC` delta and maintain a lock-free histogram. Alert on P99.9 > 2× P50 as an indicator of OS jitter, cache thrashing, or GC pressure.

---

## 12. Code Review Checklist

When reviewing any hot-path code change, verify each item:

- [ ] No heap allocation (`new`, `malloc`, container resize, `std::string` construction)
- [ ] No system calls (`read`, `write`, `ioctl`, `gettimeofday`, `clock_gettime` on hot path)
- [ ] No blocking primitives (`mutex`, `condition_variable`, `sleep`, `futex`)
- [ ] All `std::atomic` operations use the weakest correct memory order (no gratuitous `seq_cst`)
- [ ] Hot structs are `alignas(64)`; no false sharing between producer/consumer fields
- [ ] No virtual dispatch, no `dynamic_cast`, no `typeid`
- [ ] No exceptions thrown or caught on the hot path; all hot functions are `noexcept`
- [ ] All hot functions are `__attribute__((always_inline))` or verified inlined in assembly
- [ ] No `std::map`, `std::unordered_map`, `std::list`, or any node-based container on the hot path
- [ ] Lookup tables used for all bounded integer-domain computations
- [ ] TSC or hardware NIC timestamps used; not `clock_gettime`
- [ ] All new data structures are pre-sized and pre-touched at startup
- [ ] CPU affinity, SCHED_FIFO, and NUMA allocation set correctly for new threads
- [ ] Any new benchmark validates the change with P99.9 and P99.99 percentiles, not just mean

---

## 13. What This Document Does Not Cover

The following topics are out of scope for this instruction set but matter for a complete trading system:

- **Risk controls:** Pre-trade risk checks (position limits, notional limits, kill switches) run off the hot path on a separate thread. They must be non-blocking from the hot path's perspective, typically via a seqlock or atomic flag read.
- **Persistence and logging:** Use a lock-free ring buffer to hand off log records to a dedicated logging thread. Never call `write()`, `fprintf()`, or `std::cout` on the hot path.
- **Connectivity failover:** Primary/backup NIC and session failover logic runs off the hot path.
- **Strategy logic above the signal layer:** Portfolio optimization, risk model updates, and alpha research are not latency-critical and may use standard C++ idioms.
