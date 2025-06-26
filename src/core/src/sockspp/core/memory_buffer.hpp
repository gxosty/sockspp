#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sockspp
{

class MemoryBuffer
{
public:
    MemoryBuffer()
        : _ptr(nullptr), _size(0), _capacity(0) {}

    MemoryBuffer(void* ptr, size_t size, size_t capacity)
        : _ptr(ptr), _size(size), _capacity(capacity) {}

    inline void* get_ptr() const
    {
        return _ptr;
    }

    template <typename T>
    inline T as() const
    {
        return reinterpret_cast<T>(_ptr);
    }

    inline uint8_t& operator[](size_t idx) const
    {
        return reinterpret_cast<uint8_t*>(_ptr)[idx];
    }

    inline size_t copy_from(void* ptr, size_t size)
    {
        size_t copy_size = _capacity > size
            ? size
            : _capacity;

        memcpy(_ptr, ptr, copy_size);
        _size = copy_size;

        return copy_size;
    }

    inline size_t copy_from(MemoryBuffer& other)
    {
        size_t copy_size = _capacity > other.get_size()
            ? other.get_size()
            : _capacity;

        memcpy(_ptr, other.get_ptr(), copy_size);
        _size = copy_size;

        return copy_size;
    }

    inline uint8_t& operator+(size_t idx) const
    {
        return (*this)[idx];
    }

    inline size_t get_size() const
    {
        return _size;
    }

    inline size_t get_capacity() const
    {
        return _capacity;
    }

    inline void set_size(size_t size)
    {
        _size = size;
    }

protected:
    void* _ptr;
    size_t _size;
    size_t _capacity;
};

} // namespace sockspp
