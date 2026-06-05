# Benchmarks

Local benchmark notes from 2026-06-05 on this machine. Google Benchmark warned that
CPU scaling is enabled, so results are directional and machine-dependent.

Unless noted otherwise, important comparisons should be rerun with
`--benchmark_min_time=2s --benchmark_repetitions=5`, and median results should be
reported. Tables below prefer median results from repeated runs when available.
Some rows are still directional notes from local exploratory runs.

## Current main optimized path

- `FixedClob<..., FastOrderIdHash>`
- raw `ExecutionBuffer`
- `SpscRing<OrderCommand, 1 << 20>`
- batch pop with batch size 32
- SIMD scan is currently a standalone experiment, not integrated into `FixedClob`.

## 1. FixedClob add/cancel baseline

Current tree benchmarks order-level `FixedClob`. The old `std::map` L2 vs dense
`PriceLadderBook` comparison is not currently in this tree, so it was not rerun here.

| Benchmark | Throughput |
| --- | ---: |
| `FixedClob_AddOnly/1000000` | 48.2M items/s |
| `FixedClob_AddThenCancel/1000000` | 38.4M items/s |

TODO: restore or recreate the old L2 `std::map` vs dense `PriceLadderBook`
comparison if needed.

## 2. FixedOrderIndex: FastHash vs RobustHash vs unordered_map

`FastHash` is excellent for sequential IDs and collapses on bad-low-bit IDs.
`RobustHash` is steadier, but slower on sequential IDs. `unordered_map` is a
useful baseline.

| Index / pattern | Insert | Find | Erase |
| --- | ---: | ---: | ---: |
| FastIndex / sequential | 734M/s | 93.7M/s | 525M/s |
| FastIndex / bad low bits | 5.51M/s | 2.78M/s | 4.37M/s |
| RobustIndex / sequential | 65.8M/s | 27.7M/s | 43.4M/s |
| RobustIndex / bad low bits | 66.0M/s | 28.4M/s | 43.4M/s |
| unordered_map / sequential | 36.4M/s | 53.3M/s | 39.8M/s |
| unordered_map / bad low bits | 21.4M/s | 23.4M/s | 11.8M/s |

## 3. Matching: VectorExec vs RawExec vs NullExec

Hot one-level full-match path with `FastOrderIdHash`. `NullExec` measures the
matching core with no execution output. `RawExec` uses a preallocated
`ExecutionBuffer`, and is the chosen hot-path sink. `VectorExec` uses
`std::vector<Execution>` and was slower in this tiny hot path.

| Execution sink | Throughput |
| --- | ---: |
| NullExec | 73.5M/s |
| RawExec (`ExecutionBuffer`) | 42.3M/s |
| VectorExec (`std::vector<Execution>`) | 24.9M/s |

Broader direct RawExec matching notes:

| Benchmark | Throughput |
| --- | ---: |
| Fast AddOnly | 30.3M/s |
| Fast FullMatch | 52.3M/s |
| Fast SweepAskLevels (`10000`) | 43.8M/s |
| Robust AddOnly | 19.1M/s |
| Robust FullMatch | 20.0M/s |
| Robust SweepAskLevels (`10000`) | 34.0M/s |

## 4. Pipeline: baseline vs batch pop

Full pipeline, real time:

| Variant | Throughput |
| --- | ---: |
| AddOnly baseline RawExec | 15.7M/s |
| FullMatch baseline RawExec | 22.6M/s |
| FullMatch batch pop RawExec | 29.3M/s |

Batch pop is the main pipeline win. Pinning is machine/core-layout sensitive and
is kept as an experiment, not the default path.

## 5. SPSC: OrderCommand vs uint64/index queue

Queue-only real-time rows from `bench_pipeline`:

| Queue payload | Throughput |
| --- | ---: |
| `OrderCommand`, batch pop `<32>` | 26.7M/s |
| `uint64_t` | 69.8M/s |
| `uint32_t` index queue | 100M/s |

Smaller payloads are much faster in queue-only benchmarks. The index queue improved
queue-only throughput strongly, but full-pipeline results were mixed due to extra
command-array indirection. In this run, the full-pipeline index queue was close to
baseline at 23.0M/s. It is not the default full-pipeline design.

## 6. SIMD scan: scalar vs AVX2

Standalone experiment; SIMD scan is not integrated into `FixedClob` yet.
`n = 1,048,576`.

| Case | Scalar | Best AVX2 |
| --- | ---: | ---: |
| All zero | 8.85G/s | 11.9G/s (`<8>`) |
| Hit far | 8.91G/s | 11.7G/s (`<8>`) |
| Hit near | 7.41 ns | 6.12 ns (`<2>`) |
| Sparse every 64 | 10.1 ns | 6.92 ns (`<8>`) |

AVX2 helps most on long scans and sparse-hit scans. Scalar remains competitive for
very near hits.

## 7. Neutral or exploratory experiments

| Experiment | Result |
| --- | --- |
| cached SPSC | Mixed/noisy; sometimes helps queue-only, but not consistently in the full pipeline. |
| index queue | Strong queue-only improvement, mixed full-pipeline result due to extra command-array indirection. Not the main path. |
| `push_many` | Neutral here: 28.8M/s, basically tied with batch-pop-only at 29.3M/s. |
| thread pinning | No reliable win on this WSL/laptop setup; kept as an experiment only. |
