# arena-allocator

A single-header arena (bump-pointer) allocator in C++.

## Usage

### Basic Allocation

```cpp
#include "arena.hpp"

Arena arena(1024);

// Allocate the raw memory for trivial types
double* d = arena.allocate<double>();

// Assigning a value to the allocated memory
*d = 3.14;

// Reclaim all memory at once
arena.reset();
```

### Non-Trivial Types
Use `create<T>()` to construct non-trivially destructible types in the arena. Destructors are automatically called on `reset()` and `~Arena()`:
```cpp
Arena arena(1024);

std::string* s = arena.create<std::string>("Hello");

// ~std::string is called here
arena.reset()

```

### Scoped Allocation with Scratch
`Scratch` provides scoped allocations. All memory allocated through a scratch arena are automatically rolled back (with destructors called) when the scratch arena goes out of scope:
```cpp
Arena arena(1024 * 1024);

{
    Arena::Scratch scratch = arena.scratch();

    double* d = scratch.allocate<double>();
    std::string* s = scratch.create<std::string>("Hello");
} // d and s are released here, ~std::string is called automatically
```

### STL Allocator
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
```
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
```

Benchmarks run on an Apple M1 Pro. See `benchmarks/bench.cpp` for details.

## Design
- **Linear Allocation** - offset is advanced through a fixed set buffer with bitwise alignment (which offers fairly significant performance improvements over std::align) and overflow-safe bounds checking.
- **Destructor List** - For objects allocated with non-trivial destructors, a node is created for a reversed linked list which is run during the arena destructors and resets.
- **Scratch Scopes** - checkpoints the offset and destructor head, restoring both when the scratch goes out of scope.
- **STL Compatible** - `ArenaAllocator<T>` satisfies the C++ named allocator requirements for use with standard containers.
