# Architecture

This document describes the current project only. The repository is centered on a
fixed-capacity exchange-side matching core plus benchmark and profiling harnesses.
It is not yet a trader-side tick-to-trade engine.

## Current Data Flow

```text
OrderCommand stream
    -> SPSC queue / direct benchmark path
    -> FixedClob
    -> ExecutionSink
```

The direct benchmark path calls `FixedClob` without queue transport. The pipeline
benchmarks place `OrderCommand` values in an SPSC queue before the consumer thread
applies them to the book.

## Core Components

```text
FixedClob        = main fixed-capacity CLOB matching engine
OrderPool        = preallocated storage for live resting orders
PriceLevel       = FIFO queue metadata per price level
FixedOrderIndex  = OrderId -> OrderPool index lookup for cancel/erase
ExecutionSink    = output target for trade executions
SpscRing         = producer-consumer transport for pipeline benchmark
LatencyStats     = end-of-run latency percentile reporting
```

`FixedClob` owns the exchange-side book state: the `OrderPool`, bid/ask
`PriceLevel` arrays, the `FixedOrderIndex`, and best bid/ask tracking. It accepts
limit adds and cancels, performs price-time matching, and emits executions to the
provided sink.

`OrderPool` and `SpscRing` have different jobs:

```text
SPSC stores incoming commands temporarily.
OrderPool stores live resting orders inside the book.
```

The queue is transport between producer and consumer benchmark threads. The pool
is the fixed-capacity storage used by the matching engine after an order rests in
the book.

## Latency Reporting

`LatencyStats` is available for benchmark reporting, including p50/p90/p99/p999.
The current project profiles matching and pipeline latency, while full
tick-to-trade strategy-pipeline profiling remains future work.

`profile_pipeline_latency` records queue wait, consumer service, and end-to-end
latency for the SPSC pipeline. It is a profiling harness, not a replacement for
the Google Benchmark throughput targets.

## Current Boundary

`FixedClob` is the exchange-side matching core. It is useful as a matching engine
and as an exchange simulator target for future experiments, but the current tree
does not implement a trader-side local order book, strategy loop, risk checks,
order gateway, market data replay, or real networking.

## Future Trading-Stack Direction

The possible future direction is a separate trading-stack layer around the current
matching core:

```text
MarketEvent
    -> LocalOrderBook
    -> Strategy
    -> RiskCheck
    -> OrderGateway
    -> FixedClob exchange simulator
```

This is documentation of a future direction only. These trader-side components
are not implemented in the current project unless a future branch adds them.
