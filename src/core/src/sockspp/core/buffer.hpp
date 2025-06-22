#pragma once

#include "memory_buffer.hpp"

namespace sockspp
{

// Just a fixed size buffer
class Buffer : public MemoryBuffer
{
public:
    Buffer(size_t capacity);
    ~Buffer();
};

} // namespace sockspp
