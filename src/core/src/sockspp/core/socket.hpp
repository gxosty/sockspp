#pragma once

#include "buffer.hpp"

#include <string>
#include <cstdint>

namespace sockspp
{

class Socket
{
public:
    Socket() = default;
    Socket(int domain, int type, int protocol = 0);
    Socket(Socket& other) = delete;
    Socket(Socket&& other);
    ~Socket();

    static Socket open_tcp();

    void connect(const std::string& ip, uint16_t port);
    void bind(const std::string& ip, uint16_t port);

    int recv(Buffer& buffer, int flags = 0);
    int recv(char* buffer, size_t size, int flags = 0);
    int send(Buffer& buffer, int flags = 0);
    int send(const char* buffer, size_t size, int flags = 0);

    void close();
    int shutdown(int mode = -1);

    int detach();

    void operator=(const Socket&& other)
    {
        *this = std::move(other);
    }
    
private:
    int _fd;

}; // class Socket

} // namespace sockspp
