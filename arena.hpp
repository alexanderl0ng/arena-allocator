#pragma once
#include <stdexcept>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

class Arena {

public:
    class Scratch {

    public:
        explicit Scratch(Arena* arena) noexcept
            : arena_(arena), checkpoint_offset_(arena->offset_), checkpoint_dtor_(arena->destructor_head_) {}

        ~Scratch() noexcept {
            if (active_) arena_->restore(checkpoint_offset_, checkpoint_dtor_);
        }

        Scratch(const Scratch&) = delete;
        Scratch& operator=(const Scratch&) = delete;

        Scratch(Scratch&& other) noexcept
            : arena_(other.arena_),
            checkpoint_offset_(other.checkpoint_offset_),
            checkpoint_dtor_(other.checkpoint_dtor_),
            active_(other.active_) {
            other.active_ = false;
        }
        Scratch& operator=(Scratch&&) = delete;

        template <typename T>
        [[nodiscard]] T* allocate(std::size_t count = 1) {
            return arena_->allocate<T>(count);
        }

        template <typename T, typename... Args>
        [[nodiscard]] T* create(Args&&... args) {
            return arena_->create<T>(std::forward<Args>(args)...);
        }

        void release() noexcept { active_ = false; }

    private:
        Arena* arena_;
        std::size_t checkpoint_offset_;
        void* checkpoint_dtor_;
        bool active_ = true;
    };

    explicit Arena(std::size_t size)
        : capacity_(size), offset_(0) {
            if (size == 0) throw std::invalid_argument("Arena size must be greater than 0");
            buffer_ = static_cast<char*>(::operator new(size));
        }

    ~Arena() {
        if (buffer_) {
            run_destructors();
            ::operator delete(buffer_);
        }
    }

    Arena(const Arena&) = delete;
    Arena(Arena&& other) noexcept
        : capacity_(other.capacity_), offset_(other.offset_), buffer_(other.buffer_), destructor_head_(other.destructor_head_) {
            other.capacity_ = 0;
            other.offset_ = 0;
            other.buffer_ = nullptr;
            other.destructor_head_ = nullptr;
        }

    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            run_destructors();
            ::operator delete(buffer_);
            capacity_ = other.capacity_;
            offset_ = other.offset_;
            buffer_ = other.buffer_;
            destructor_head_ = other.destructor_head_;
            other.capacity_ = 0;
            other.offset_ = 0;
            other.buffer_ = nullptr;
            other.destructor_head_ = nullptr;
        }
        return *this;
    }

    template <typename T>
    [[nodiscard]] T* allocate(std::size_t count = 1) {
        if (count > (std::numeric_limits<std::size_t>::max() / sizeof(T))) throw std::bad_alloc();
        void* ptr = allocate_raw(sizeof(T) * count, alignof(T));
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    template <typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args) {
        std::size_t saved = offset_;
        T* ptr = allocate<T>();

        DestructorNode* node = nullptr;
        if constexpr (!std::is_trivially_destructible_v<T>) node = allocate<DestructorNode>();

        try {
            ::new(ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            offset_ = saved;
            throw;
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            node->destructor = [](void* p) { static_cast<T*>(p)->~T(); };
            node->object = ptr;
            node->next = destructor_head_;
            destructor_head_ = node;
        }

        return ptr;
    }

    [[nodiscard]] Arena::Scratch scratch() noexcept {
        return Arena::Scratch(this);
    }

    void reset() noexcept {
        if (destructor_head_) run_destructors();
        offset_ = 0;
    }

    std::size_t bytes_used() const noexcept { return offset_; }
    std::size_t bytes_remaining() const noexcept { return capacity_ - offset_; }
    std::size_t bytes_reserved() const noexcept { return capacity_; }

private:
    friend class Scratch;

    struct DestructorNode {
        void (*destructor)(void*);
        void* object;
        DestructorNode* next;
    };

    std::size_t capacity_;
    std::size_t offset_;
    char* buffer_ = nullptr;
    DestructorNode* destructor_head_ = nullptr;

    void* allocate_raw(std::size_t size, std::size_t alignment) noexcept {
        std::size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);

        if (aligned_offset < offset_) return nullptr;
        if (aligned_offset > capacity_ || size > capacity_ - aligned_offset) return nullptr;

        void* ptr = buffer_ + aligned_offset;
        offset_ = aligned_offset + size;
        return ptr;
    }

    void run_destructors() noexcept {
        DestructorNode* node = destructor_head_;
        while (node) {
            node->destructor(node->object);
            node = node->next;
        }
        destructor_head_ = nullptr;
    }

    void restore(std::size_t offset, void* dtor_head) noexcept {
        if (destructor_head_ != dtor_head) {
            DestructorNode* node = destructor_head_;
            DestructorNode* target = static_cast<DestructorNode*>(dtor_head);
            while (node != target) {
                node->destructor(node->object);
                node = node->next;
            }
            destructor_head_ = target;
        }
        offset_ = offset;
    }
};

template <typename T>
class ArenaAllocator {

public:
    using value_type = T;

    template <typename U>
    struct rebind {
        using other = ArenaAllocator<U>;
    };

    explicit ArenaAllocator(Arena* arena) noexcept : arena_(arena) {}

    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) noexcept : arena_(other.arena_) {}

    template <typename U>
    friend class ArenaAllocator;

    T* allocate(std::size_t n) {
        return arena_->allocate<T>(n);
    }

    void deallocate(T* /*p*/, std::size_t /*n*/) noexcept {}

    bool operator==(const ArenaAllocator& other) const noexcept {
        return arena_ == other.arena_;
    }

    bool operator!=(const ArenaAllocator& other) const noexcept {
        return !(*this == other);
    }

private:
    Arena* arena_;
};

