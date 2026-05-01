#pragma once
#include <stdexcept>
#include <cstddef>
#include <new>

class Arena {

    public:
        explicit Arena(std::size_t size)
            : buffer_(allocate_buffer(size)),
                        capacity_(size), offset_(0) {}

        ~Arena() {
            ::operator delete(buffer_);
        }

        Arena(const Arena&) = delete;
        Arena(Arena&&) = delete;

        template<typename T>
        [[nodiscard]] T* allocate(std::size_t count = 1) {
            void* ptr = allocate_raw(sizeof(T) * count, alignof(T));
            if (!ptr) throw std::bad_alloc();
            return static_cast<T*>(ptr);
        }

        void reset() noexcept { offset_ = 0; }

    private:
        static char* allocate_buffer(std::size_t size) {
            if (size == 0) throw std::invalid_argument("Arena size must be greater than 0");
            return static_cast<char*>(::operator new(size));
        }

        void* allocate_raw(std::size_t size, std::size_t alignment) noexcept {
            std::size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);

            if (aligned_offset < offset_) return nullptr;
            if (size > capacity_ - aligned_offset) return nullptr;

            void* ptr = buffer_ + aligned_offset;
            offset_ = aligned_offset + size;
            return ptr;
        }

        std::size_t save() const noexcept { return offset_; }
        void restore(std::size_t checkpoint) noexcept { offset_ = checkpoint; }

        friend class ArenaScratch;
       
        char* buffer_;
        std::size_t capacity_;
        std::size_t offset_;
};

class ArenaScratch {
    public:
        explicit ArenaScratch(Arena* arena) noexcept
            : arena_(arena), checkpoint_(arena->save()) {}

        ~ArenaScratch() noexcept {
            if (active_) arena_->restore(checkpoint_);
        }

        ArenaScratch(const ArenaScratch&) = delete;
        ArenaScratch& operator=(const ArenaScratch&) = delete;

        ArenaScratch(ArenaScratch&& other) noexcept
            : arena_(other.arena_), checkpoint_(other.checkpoint_), active_(other.active_) {
            other.active_ = false;
        }
        ArenaScratch& operator=(ArenaScratch&&) = delete;

        void release() noexcept { active_ = false; }

    private:
        Arena* arena_;
        std::size_t checkpoint_;
        bool active_ = true;
};

template <typename T>
class ArenaAllocator {

    public:
        using value_type = T;

        explicit ArenaAllocator(Arena* arena) noexcept : arena_(arena) {}

        template <typename U>
        ArenaAllocator(const ArenaAllocator<U>& other) noexcept : arena_(other.arena_) {}

        template <typename U>
        friend class ArenaAllocator;

        T* allocate(std::size_t n) {
            T* ptr = static_cast<T*>(arena_->allocate<T>(n));
            if (!ptr) throw std::bad_alloc();
            return ptr;
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

