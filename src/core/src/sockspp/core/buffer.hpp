#pragma once

#include <cstddef>

namespace sockspp
{

// Just a fixed size buffer
class Buffer
{
public:
    Buffer(size_t capacity);
    ~Buffer();

    inline void* get_data() const
    {
        return _data;
    }

    inline char* cstr() const
    {
        return reinterpret_cast<char*>(_data);
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

private:
    void* _data;
    size_t _size;
    size_t _capacity;
};

} // namespace sockspp
