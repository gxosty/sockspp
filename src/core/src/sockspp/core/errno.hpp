#pragma once

#include <cerrno>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #define sockerrno WSAGetLastError()
#else
    #define sockerrno errno
#endif
