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
ArenaAllocator<std::pair<const int, long long>> alloc(&arena);

std::unordered_map<int, long long,
    std::hash<int>,
    std::equal_to<int>,
    ArenaAllocator<std::pair<const int, long long>>
> map(0, std::hash<int>(), std::equal_to<int>(), alloc);
```

### Moving the Arena
The arena can be moved, it cannot be copied. Using the move from arena is undefined behaviour since it is not "dead", it is still reusable. It will throw `std::bad_alloc` if an allocation is attempted.

## Benchmarks
```
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_NewDouble                            8.99 ns         8.99 ns     80029268
BM_ArenaDouble                         0.226 ns        0.226 ns   3098387068
BM_UnorderedMap_Insert/100              3500 ns         3500 ns       199325
BM_UnorderedMap_Insert/1000            34346 ns        34346 ns        20228
BM_UnorderedMap_Insert/10000          368759 ns       368753 ns         1967
BM_ArenaUnorderedMap_Insert/100         2355 ns         2354 ns       299770
BM_ArenaUnorderedMap_Insert/1000       21724 ns        21723 ns        32456
BM_ArenaUnorderedMap_Insert/10000     233644 ns       233640 ns         3049
BM_Vector_Insert/100                     174 ns          174 ns      4001143
BM_Vector_Insert/1000                    554 ns          554 ns      1276836
BM_Vector_Insert/10000                  6928 ns         6928 ns        99816
BM_ArenaVector_Insert/100               58.2 ns         58.2 ns     12159956
BM_ArenaVector_Insert/1000               358 ns          358 ns      1967342
BM_ArenaVector_Insert/10000             4997 ns         4997 ns       138192
```

Benchmarks run on an Apple M4. See `benchmarks/bench.cpp` for details.

## Design
- **Linear Allocation** - offset is advanced through a fixed set buffer with bitwise alignment (which offers fairly significant performance improvements over std::align) and overflow-safe bounds checking.
- **Destructor List** - For objects allocated with non-trivial destructors, a node is created for a reversed linked list which is run during the arena destructors and resets.
- **Scratch Scopes** - checkpoints the offset and destructor head, restoring both when the scratch goes out of scope.
- **STL Compatible** - `ArenaAllocator<T>` satisfies the C++ named allocator requirements for use with standard containers.
