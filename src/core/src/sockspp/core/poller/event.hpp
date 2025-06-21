#pragma once

#if defined(_WIN32) || defined(__linux__)
    #include <wepoll.h>
    #define SOCKSPP_POLLIN EPOLLIN
    #define SOCKSPP_POLLOUT EPOLLOUT
    #define SOCKSPP_POLLHUP EPOLLHUP
#endif

namespace sockspp
{

class Event
{
public:
    enum Flags : uint32_t
    {
        Read = SOCKSPP_POLLIN,
        Write = SOCKSPP_POLLOUT,
        Closed = SOCKSPP_POLLHUP
    }; // enum class Flags

public:
    Event(int fd, Flags flags, void* ptr)
    : _fd(fd)
    , _flags(flags)
    , _ptr(ptr) {}

    inline int get_fd() const
    {
        return _fd;
    }

    inline Flags get_flags() const
    {
        return _flags;
    }

    inline void* get_ptr() const
    {
        return _ptr;
    }

private:
    void* _ptr;
    Flags _flags;
    int _fd;
}; // class Event

} // namespace sockspp
