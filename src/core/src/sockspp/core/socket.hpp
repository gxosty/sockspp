#pragma once

#include "buffer.hpp"

#include <string>
#include <cstdint>

namespace sockspp
{

class Socket
{
public:
    Socket(int domain, int type, int protocol = 0);
    Socket(Socket&& other);
    ~Socket();

    int connect(const std::string& ip, uint16_t port);

    int recv(Buffer& buffer, int flags = 0);
    int recv(char* buffer, size_t size, int flags = 0);
    int send(Buffer& buffer, int flags = 0);
    int send(const char* buffer, size_t size, int flags = 0);

    void close();
    int shutdown(int mode = -1);

    int detach();
    
private:
    int _fd;

}; // class Socket

} // namespace sockspp
