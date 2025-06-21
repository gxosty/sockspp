#pragma once

#if defined(_WIN32) || defined(__linux__)
    #include "epoll_poller.hpp"

    namespace sockspp
    {
        using Poller = sockspp::EpollPoller;
    }
#else
    #error "Unsupported platform"
#endif
