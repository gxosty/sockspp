#pragma once

#include "buffer.hpp"

#include <string>
#include <cstdint>

namespace sockspp
{

struct SocketInfo;

class Socket
{
public:
    Socket() = default;
    Socket(int domain, int type, int protocol = 0);
    Socket(int fd);
    Socket(Socket& other) = delete;
    Socket(Socket&& other);
    ~Socket();

    static Socket open_tcp();

    bool set_blocking(bool enabled);

    void connect(const std::string& ip, uint16_t port);
    void bind(const std::string& ip, uint16_t port);
    void listen(int count);
    Socket accept(SocketInfo* info = nullptr);

    int recv(Buffer& buffer, int flags = 0);
    int recv(char* buffer, size_t size, int flags = 0);
    int send(Buffer& buffer, int flags = 0);
    int send(const char* buffer, size_t size, int flags = 0);

    void close();
    int shutdown(int mode = -1);

    int get_fd() const;
    int detach();

    void operator=(Socket&& other)
    {
        _fd = other.detach();
    }
    
private:
    int _fd;

}; // class Socket

struct SocketInfo
{
    enum IPVersion : uint8_t
    {
        IPv4,
        IPv6
    };

    uint8_t ip[16];
    uint16_t port;
    IPVersion ip_version;

    std::string str() const;
}; // struct SocketInfo

} // namespace sockspp
