#include <unordered_map>
#include <vector>
#include <random>
#include <benchmark/benchmark.h>
#include "../arena.hpp"

static void BM_NewDouble(benchmark::State& state) {
    for (auto _ : state) {
        double* d = new double(3.14);
        benchmark::DoNotOptimize(d);
        delete d;
    }
}

static void BM_ArenaDouble(benchmark::State& state) {
    Arena arena(1024 * 16);
    for (auto _ : state) {
        double* d = arena.allocate<double>();
        benchmark::DoNotOptimize(d);
        arena.reset();
    }
}

static void BM_UnorderedMap_Insert(benchmark::State& state) {
    std::size_t num_elements = state.range(0);

    std::mt19937 rng(79);
    std::uniform_int_distribution<int> dist(1, 1'000'000);

    for (auto _ : state) {
        std::unordered_map<int, long long> map;
        for (std::size_t i = 0; i < num_elements; ++i) {
            int key = dist(rng);
            map[key] = i;
        }

    benchmark::DoNotOptimize(map);
    }
}

static void BM_ArenaUnorderedMap_Insert(benchmark::State& state) {
    std::size_t num_elements = state.range(0);

    std::mt19937 rng(79);
    std::uniform_int_distribution<int> dist(1, 1'000'000);

    Arena arena(1024 * 1024);
    for (auto _ : state) {
        Arena::Scratch scratch = arena.scratch();
        ArenaAllocator<std::pair<int, long long>> alloc(&arena);
        std::unordered_map<int, long long,
            std::hash<int>,
            std::equal_to<int>,
            ArenaAllocator<std::pair<const int, long long>>
        > map(
            0,
            std::hash<int>(),
            std::equal_to<int>(),
            alloc
        );
        for (std::size_t i = 0; i < num_elements; ++i) {
            int key = dist(rng);
            map[key] = i;
        }

        benchmark::DoNotOptimize(map);
    }
}

static void BM_Vector_Insert(benchmark::State& state) {
    std::size_t num_elements = state.range(0);

    for (auto _ : state) {
        std::vector<long long> vec;
        for (std::size_t i = 0; i < num_elements; ++i) {
            vec.push_back(i);
        }

    benchmark::DoNotOptimize(vec);
    }
}

static void BM_ArenaVector_Insert(benchmark::State& state) {
    std::size_t num_elements = state.range(0);

    Arena arena(1024 * 1024);
    for (auto _ : state) {
        Arena::Scratch scratch = arena.scratch();
        ArenaAllocator<long long> alloc(&arena);

        std::vector<long long, ArenaAllocator<long long>> vec(alloc);
        for (std::size_t i = 0; i < num_elements; ++i) {
            vec.push_back(i);
        }

    benchmark::DoNotOptimize(vec);
    }
}

BENCHMARK(BM_NewDouble);
BENCHMARK(BM_ArenaDouble);

BENCHMARK(BM_UnorderedMap_Insert)
    ->Arg(100)
    ->Arg(1'000)
    ->Arg(10'000);

BENCHMARK(BM_ArenaUnorderedMap_Insert)
    ->Arg(100)
    ->Arg(1'000)
    ->Arg(10'000);

BENCHMARK(BM_Vector_Insert)
    ->Arg(100)
    ->Arg(1'000)
    ->Arg(10'000);

BENCHMARK(BM_ArenaVector_Insert)
    ->Arg(100)
    ->Arg(1'000)
    ->Arg(10'000);

BENCHMARK_MAIN();
