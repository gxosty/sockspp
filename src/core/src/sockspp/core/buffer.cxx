#include "sockspp/core/memory_buffer.hpp"
#include <sockspp/core/buffer.hpp>
#include <sockspp/core/exceptions.hpp>

#include <cstdlib>

namespace sockspp
{

Buffer::Buffer()
{
    _ptr = nullptr;
    _capacity = 0;
    _size = 0;
}

Buffer::Buffer(size_t capacity) : MemoryBuffer()
{
    _ptr = malloc(capacity);

    if (!_ptr)
    {
        throw MemoryAllocationException(capacity);
    }

    *(char*)_ptr = 0;
    _capacity = capacity;
}

Buffer::~Buffer()
{
    if (_ptr)
        free(_ptr);
}

} // namespace sockspp
