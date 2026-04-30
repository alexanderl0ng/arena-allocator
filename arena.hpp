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

