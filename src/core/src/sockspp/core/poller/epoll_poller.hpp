#pragma once

#include "event.hpp"
#include "../exceptions.hpp"

#ifdef _WIN32
    #include <wepoll.h>
    #define WINDOWS_LEAN_AND_MEAN
    #include <windows.h>
    typedef HANDLE epoll_handle_t;
    #define SOCKSPP_INVALID_POLLER INVALID_HANDLE_VALUE
#else
    #include <sys/epoll.h>
    typedef int epoll_handle_t;
    #define SOCKSPP_INVALID_POLLER -1
    #define epoll_close(fd) close(fd)
#endif // _WIN32

#define POLL_BATCH 512

#include <vector>

namespace sockspp
{

class EpollPoller
{
public:
    EpollPoller()
    : _fd(epoll_create1(0))
    , _poll_batch(POLL_BATCH)
    {
        if (_fd == SOCKSPP_INVALID_POLLER)
        {
            throw PollerCreationException();
        }
    }

    ~EpollPoller()
    {
        if (_fd != SOCKSPP_INVALID_POLLER)
        {
            epoll_close(_fd);
            _fd = SOCKSPP_INVALID_POLLER;
        }
    }

    bool register_event(const Event& event)
    {
        epoll_event ev;
        ev.events = event.get_flags();
        ev.data.ptr = event.get_ptr();
        if (epoll_ctl(_fd, EPOLL_CTL_ADD, event.get_fd(), &ev))
        {
            return false;
        }

        return true;
    }

    bool update_event(const Event& event)
    {
        epoll_event ev;
        ev.events = event.get_flags();
        ev.data.ptr = event.get_ptr();
        if (epoll_ctl(_fd, EPOLL_CTL_MOD, event.get_fd(), &ev))
        {
            return false;
        }

        return true;
    }

    bool remove_event(const Event& event)
    {
        if (epoll_ctl(_fd, EPOLL_CTL_DEL, event.get_fd(), nullptr))
        {
            return false;
        }

        return true;
    }

    int poll(std::vector<Event>& out_events, int timeout)
    {
        epoll_event events[_poll_batch];
        memset((void*)events, 0, sizeof(events));

        int ndfs = epoll_wait(_fd, events, _poll_batch, timeout);

        for (int i = 0; i < ndfs; i++)
        {
            auto event = events[i];
            out_events.emplace_back(
                0,  // I am shocked but that's literally how epoll works
                static_cast<Event::Flags>(event.events),
                event.data.ptr
            );
        }
        
        return ndfs;
    }

private:
    epoll_handle_t _fd;
    int _poll_batch;
}; // class EpollPoller

} // namespace sockspp
