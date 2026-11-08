#ifndef NANO_EDR_RING_BUFFER_H
#define NANO_EDR_RING_BUFFER_H

#include <array>
#include <cstddef>

namespace nano_edr {

template <typename T, std::size_t N>
class RingBuffer {
    static_assert(N > 0);

 public:
    class Iterator {
     public:
        Iterator(const RingBuffer* buffer, std::size_t index)
            : buffer_(buffer), index_(index) {}

        const T& operator*() const { return (*buffer_)[index_]; }
        Iterator& operator++() {
            ++index_;
            return *this;
        }
        bool operator!=(const Iterator& other) const {
            return buffer_ != other.buffer_ || index_ != other.index_;
        }

     private:
        const RingBuffer* buffer_ = nullptr;
        std::size_t index_ = 0;
    };

    void PushBack(const T& value) {
        data_[(oldest_ + size_) % N] = value;
        if (full()) {
            oldest_ = (oldest_ + 1) % N;
        } else {
            ++size_;
        }
    }

    void Clear() {
        oldest_ = 0;
        size_ = 0;
    }

    std::size_t size() const { return size_; }
    static constexpr std::size_t capacity() { return N; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    const T& operator[](std::size_t index) const {
        return data_[(oldest_ + index) % N];
    }
    const T& back() const { return (*this)[size_ - 1]; }
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, size_); }

 private:
    std::array<T, N> data_{};
    std::size_t oldest_ = 0;
    std::size_t size_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_RING_BUFFER_H
