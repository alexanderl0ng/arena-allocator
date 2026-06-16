#include "../arena.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <iostream>

static std::vector<int> g_events;
struct Tracked {
    int id;
    explicit Tracked(int id) : id(id) { g_events.push_back(+id); }
    ~Tracked()                        { g_events.push_back(-id); }
};
struct ThrowingConstructor {
    ThrowingConstructor() { throw std::runtime_error("fail"); }
    ~ThrowingConstructor() { g_events.push_back(-999); }
};
struct alignas(16) Aligned16 { char data[16]; };
struct TestSection {
    std::string_view name;
    TestSection(std::string_view name) : name(name) {
        std::clog << "[ RUN      ] " << name << '\n';
    }
    ~TestSection() {
        std::clog << "[     DONE ] " << name << '\n';
    }
};


int main() {
    {
    TestSection("General Allocation");
    Arena a(1024);
    assert(a.allocate<int>() != nullptr);
    auto* p = a.allocate<Aligned16>();
    assert(reinterpret_cast<std::uintptr_t>(p) % 16 == 0);
    int* x = a.allocate<int>();
    *x = 1;
    int* y = a.allocate<int>();
    *y = 2;
    assert(*x == 1 && *y == 2);
    }

    {
    TestSection("Exhaustion");
    Arena a(sizeof(int));
    std::ignore = a.allocate<int>();
    bool threw = false;
    try { std::ignore = a.allocate<int>(); } catch (const std::bad_alloc&) { threw = true; }
    assert(threw);
    }

    {
    TestSection("create<T> and destructor ordering");
    g_events.clear();
        {
        Arena a(2048);
        std::ignore = a.create<Tracked>(1);
        std::ignore = a.create<Tracked>(2);
        std::ignore = a.create<Tracked>(3);
        }
    assert(g_events.size() == 6);
    assert(g_events[3] == -3 && g_events[4] == -2 && g_events[5] == -1);
    }

    {
    TestSection("Exception in constructor");
    g_events.clear();
    Arena a(1024);
    std::size_t before = a.bytes_used();
    try { std::ignore = a.create<ThrowingConstructor>(); } catch (...) {}
    assert(a.bytes_used() == before);
    a.reset();
    for (int ev : g_events) assert(ev != -999);
    }

    {
    TestSection("Scratch allocations");
    g_events.clear();
    Arena a(4096);
    std::ignore = a.create<Tracked>(1);
    std::size_t base = a.bytes_used();
    {
        auto s = a.scratch();
        std::ignore = s.create<Tracked>(2);
        std::ignore = s.allocate<int>(20);
    }
    assert(a.bytes_used() == base);
    int destructors = 0;
    for (int ev : g_events) if (ev < 0) ++destructors;
    assert(destructors == 1 && g_events.back() == -2);
    }

    {
    TestSection("Nested scratch allocations");
    Arena a(4096);
    auto outer = a.scratch();
    std::ignore = outer.allocate<int>(10);
    std::size_t mid = a.bytes_used();
        {
        auto inner = a.scratch();
        std::ignore = inner.allocate<int>(10);
        assert(a.bytes_used() > mid);
        }
    assert(a.bytes_used() == mid);
    }

    {
    TestSection("Moving the arena");
    Arena a(1024);
    int* p = a.allocate<int>();
    *p = 42;
    Arena b(std::move(a));
    assert(*p == 42);
    assert(a.bytes_reserved() == 0);
    }

    {
    TestSection("STL Allocator");
    Arena a(1024 * 64);
    ArenaAllocator<int> alloc(&a);
    std::vector<int, ArenaAllocator<int>> v(alloc);
    for (int i = 0; i < 100; ++i) v.push_back(i);
    for (int i = 0; i < 100; ++i) assert(v[i] == i);
    }
}

