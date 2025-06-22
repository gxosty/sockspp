#include "sockspp/core/memory_buffer.hpp"
#include <sockspp/core/buffer.hpp>
#include <sockspp/core/exceptions.hpp>

#include <cstdlib>

namespace sockspp
{

Buffer::Buffer(size_t capacity)
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
    free(_ptr);
}

} // namespace sockspp
