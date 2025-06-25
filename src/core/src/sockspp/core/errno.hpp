#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #define sockerrno WSAGetLastError()
#else
    #include <errno.h>
    #include <cstring>
    #define sockerrno errno
#endif
