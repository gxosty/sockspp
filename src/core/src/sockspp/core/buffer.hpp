#pragma once

#include "memory_buffer.hpp"

namespace sockspp
{

// Just a fixed size buffer (in heap)
class Buffer : public MemoryBuffer
{
public:
    Buffer();
    Buffer(size_t capacity);
    ~Buffer();
};

} // namespace sockspp
