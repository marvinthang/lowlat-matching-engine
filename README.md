# Low-Latency Matching Engine

A C++20 performance engineering project implementing an order-level central limit
order book with fixed-capacity memory pools, index-based price-level queues, a
custom open-addressing order index, execution sinks, and an SPSC producer-consumer
pipeline.

The goal is not to build a production exchange, but to study how low-latency C++
systems are designed, measured, and optimized.

## Features

* Order-level central limit order book (`FixedClob`)
* FIFO price-time priority at each price level
* Limit order submission with full and partial matching
* Order cancellation by `OrderId`
* Execution reports through pluggable sinks
* Fixed-capacity `OrderPool` with free-list reuse
* Fixed open-addressing `OrderId -> OrderIndex` table
* Fast and robust order-id hash policy variants
* SPSC queue for producer-consumer pipeline benchmarks
* Pipeline benchmarks with raw execution reporting and batch pop
* AVX2 scan experiment for next non-zero level search
* Google Benchmark microbenchmarks and local benchmark notes

## Architecture

```text
OrderCommand stream
        |
        v
SPSC queue
        |
        v
FixedClob matching engine
        |
        +--> OrderPool
        |       fixed-capacity order storage
        |
        +--> PriceLevel arrays
        |       FIFO queues by price
        |
        +--> FixedOrderIndex
        |       OrderId -> OrderIndex lookup
        |
        v
Execution sink
```

## Core Data Structures

### `OrderPool`

`OrderPool` stores all live orders in a preallocated array and recycles slots
through a free stack.

This avoids per-order heap allocation on the hot path.

```text
allocate -> returns OrderIndex
release  -> recycles OrderIndex
```

### `PriceLevel`

Each price level stores only metadata:

```text
head_idx
tail_idx
total_qty
order_count
```

Orders at the same price are linked through `next_idx` and `prev_idx` inside the
order pool.

This gives FIFO priority without allocating list nodes.

### `FixedOrderIndex`

`FixedOrderIndex` maps:

```text
OrderId -> OrderIndex
```

It uses open addressing over fixed arrays.

Two hash policies are supported:

```text
FastOrderIdHash
RobustOrderIdHash
```

`FastOrderIdHash` is very fast for sequential IDs but performs poorly on bad
low-bit ID patterns. `RobustOrderIdHash` is steadier for arbitrary IDs but slower
on clean sequential IDs.

### `FixedClob`

`FixedClob` owns the full order book:

```text
OrderPool
bid/ask PriceLevel arrays
FixedOrderIndex
best bid / best ask
```

It supports:

```text
add_order
cancel_order
submit_limit_order
best_bid
best_ask
quantity_at
```

### Execution Sinks

The matching engine supports different execution report sinks:

```text
std::vector<Execution>   convenient for tests/debugging
ExecutionBuffer          preallocated raw hot-path sink
NullExecutionSink        measures matching core without output
```

The raw `ExecutionBuffer` is the preferred hot-path sink.

## Pipeline

The pipeline benchmark connects the engine to a producer-consumer setup:

```text
producer thread
    pushes OrderCommand into SPSC queue

consumer thread
    pops commands
    applies them to FixedClob
```

The current optimized pipeline path uses:

```text
FixedClob<..., FastOrderIdHash>
raw ExecutionBuffer
SpscRing<OrderCommand, 1 << 20>
batch pop with batch size 32
```

## SIMD Experiment

The project includes a standalone AVX2 scan experiment:

```text
scalar_find_next_nonzero
avx2_find_next_nonzero
```

This is intended to study SIMD scanning over compact price-level occupancy arrays.

It is not currently integrated into `FixedClob`.

## Build

This CMake file expects Google Benchmark to be available as a sibling checkout at
`../benchmark`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Some SIMD benchmarks/tests require AVX2 support and are compiled with `-mavx2`.

## Run Tests

```bash
./build/test_order_pool
./build/test_order_index
./build/test_fixed_clob
./build/test_matching
./build/test_spsc_ring
./build/test_spsc_cached
./build/test_simd_scan
```

## Run Benchmarks

Examples:

```bash
./build/bench_fixed_clob --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
./build/bench_order_index --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
./build/bench_matching --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
./build/bench_matching_hot --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
./build/bench_pipeline --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
./build/bench_simd_scan --benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
```

For local notes, see:

```text
BENCHMARKS.md
```

## Benchmark Notes

Benchmark numbers are machine-dependent. Google Benchmark warned that CPU scaling
was enabled on the local machine, so the numbers should be treated as directional
engineering notes, not publishable measurements.

Important comparisons should be run with:

```bash
--benchmark_min_time=2s --benchmark_repetitions=5 --benchmark_display_aggregates_only=true
```

and median results should be used when repetitions are available.

## Optimization Results

The main lessons from the current benchmark work:

* Fixed open-addressing lookup is much faster than `unordered_map` for clean
  sequential IDs.
* Hash choice matters: `FastHash` collapses on bad low-bit IDs, while
  `RobustHash` is steadier.
* Raw execution reporting through `ExecutionBuffer` is much faster than using
  `std::vector<Execution>` in the tiny matching hot path.
* Batch pop improves SPSC pipeline throughput.
* Queueing smaller payloads is much faster in queue-only benchmarks, but index
  queueing did not become the default full-pipeline design due to extra command
  indirection.
* Cached SPSC, producer-side `push_many`, and thread pinning were mixed or neutral
  on this WSL/laptop setup.
* AVX2 helps most on long scans and sparse-hit scans; scalar remains competitive
  when the hit is very near.

## Limitations

This is a learning/performance engineering project, not a production trading
system.

Current limitations:

* Synthetic workloads only
* No real market data protocol parser
* No persistence or recovery
* No risk checks
* No networking
* No multi-symbol sharding
* No latency histogram or p99 reporting yet
* SIMD scan is standalone and not integrated into `FixedClob`
* Benchmarks were run on a local laptop/WSL setup with CPU scaling enabled

## Future Work

Possible next steps:

* Add latency histogram measurements
* Add direct native Linux benchmark runs with controlled CPU governor
* Add thread pinning experiments on a more stable setup
* Add real feed/replay parser
* Add multi-symbol engine sharding
* Integrate occupancy-array best-price refresh and test scalar vs AVX2 refresh
* Add more realistic mixed add/cancel/match workloads
* Add benchmark reports for p50/p99 latency, not only throughput

## Project Status

Current stable path:

```text
FixedClob
FastOrderIdHash
ExecutionBuffer
SpscRing<OrderCommand>
BatchPop<32>
```

Current experimental modules:

```text
RobustOrderIdHash
SpscRingCached
IndexQueue pipeline
push_many
AVX2 scan
```
