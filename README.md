# arena-allocator

A single-header arean allocator in C++.

## Usage

```cpp
#include "arena.hpp"

Arena arena(1024);
double* d = arena.allocate<double>();
arena.reset();
```

## Design
Bump allocator with bitwise alignment calculation and overflow-safe bounds checking.

