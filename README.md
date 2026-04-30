# arena-allocator

A single-header arena allocator in C++.

## Usage

```cpp
#include "arena.hpp"

Arena arena(1024);
double* d = arena.allocate<double>();
arena.reset();
```

## STL Allocator
`arena.hpp` also includes `ArenaAllocator`, an STL-compatible wrapper for use with standard containers:

```cpp
Arena arena(1024 * 1024);
ArenaAllocator<std::pair<const int, long long> alloc(&arena);
std::unordered_map<int, long long,
    std::hash<int>,
    std::equal_to<int>,
    ArenaAllocator<std::pair<const int, long long>>
> map(0, std::hash<int>(), std::equal_to<int>(), alloc);
```

## Benchmarks
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_New                                  14.2 ns         14.2 ns     49358342
BM_Arena                               0.310 ns        0.310 ns   2255932296
BM_UnorderedMap_Insert/100              5257 ns         5257 ns       131310
BM_UnorderedMap_Insert/1000            52151 ns        52148 ns        13346
BM_UnorderedMap_Insert/10000          511518 ns       511513 ns         1359
BM_ArenaUnorderedMap_Insert/100         3496 ns         3496 ns       200274
BM_ArenaUnorderedMap_Insert/1000       31003 ns        31003 ns        22630
BM_ArenaUnorderedMap_Insert/10000     322900 ns       322892 ns         2168

Benchmarks run on an Apple M1 Pro. See `benchmarks/bench.cpp` for details.

## Design
Bump allocator with bitwise alignment calculation and overflow-safe bounds checking.

