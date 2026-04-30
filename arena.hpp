#pragma once
#include <cstddef>
#include <new>

class Arena {

    public:
        explicit Arena(std::size_t size)
            : buffer_(static_cast<char*>(::operator new(size))),
                        capacity_(size), offset_(0) {}

        ~Arena() {
            ::operator delete(buffer_);
        }

        Arena(const Arena&) = delete;
        Arena(Arena&&) = delete;

        template<typename T>
        T* allocate(std::size_t count = 1) noexcept {
            void* ptr = allocate_raw(sizeof(T) * count, alignof(T));
            return static_cast<T*>(ptr);
        }

        void reset() noexcept { offset_ = 0; }

    private:

        void* allocate_raw(std::size_t size, std::size_t alignment) noexcept {
            std::size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);

            if (size > capacity_ - aligned_offset) return nullptr;

            void* ptr = buffer_ + aligned_offset;
            offset_ = aligned_offset + size;
            return ptr;
        }
        
        char* buffer_;
        std::size_t capacity_;
        std::size_t offset_;
};

template <typename T>
class ArenaAllocator{

    public:
        using value_type = T;

        explicit ArenaAllocator(Arena* arena) noexcept : arena_(arena) {}

        template <typename U>
        ArenaAllocator(const ArenaAllocator<U>& other) noexcept : arena_(other.arena_) {}

        template <typename U>
        friend class ArenaAllocator;

        T* allocate(std::size_t n) {
            return static_cast<T*>(arena_->allocate<T>(n));
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

