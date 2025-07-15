#pragma once

#include <cerrno>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #define sockerrno WSAGetLastError()

    #define SOCKSPP_EWOULDBLOCK WSAEWOULDBLOCK
    #define SOCKSPP_EAGAIN WSAEWOULDBLOCK
    #define SOCKSPP_EINPROGRESS WSAEINPROGRESS
#else
    #define sockerrno errno

    #define SOCKSPP_EWOULDBLOCK EWOULDBLOCK
    #define SOCKSPP_EAGAIN EWOULDBLOCK
    #define SOCKSPP_EINPROGRESS EINPROGRESS
#endif
