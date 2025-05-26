#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #define sockerrno WSAGetLastError()
#else
    #include <errno.h>
    #define sockerrno errno
#endif
