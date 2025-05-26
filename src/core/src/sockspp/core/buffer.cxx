#include <sockspp/core/buffer.hpp>
#include <sockspp/core/exceptions.hpp>

#include <cstdlib>

namespace sockspp
{

Buffer::Buffer(size_t capacity) : _size{0}
{
    _data = malloc(capacity);

    if (!_data)
    {
        throw MemoryAllocationException(capacity);
    }

    *(char*)_data = 0;
    _capacity = capacity;
}

Buffer::~Buffer()
{
    free(_data);
}

} // namespace sockspp
