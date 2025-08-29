#pragma once

#include "memory_buffer.hpp"

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
    static Socket open_tcp6();
    static Socket open_udp();
    static Socket open_udp6();

    bool set_blocking(bool enabled);
    bool set_nodelay(bool enabled);
    bool set_keepalive(bool enabled);

    void connect(const std::string& ip, uint16_t port);
    int connect(void* sock_addr, int sock_addr_len);
    void bind(const std::string& ip, uint16_t port);
    int bind(void* sock_addr, int sock_addr_len);
    void listen(int count);
    Socket accept(SocketInfo* info = nullptr);

    int recv(MemoryBuffer& buffer, int flags = 0);
    int recv(char* buffer, size_t size, int flags = 0);
    int recv_from(MemoryBuffer& buffer, SocketInfo* info = nullptr, int flags = 0);
    int recv_from(
        char* buffer,
        size_t size,
        void* sock_addr,
        int* sock_addr_len,
        int flags = 0
    );

    int send(MemoryBuffer& buffer, int flags = 0);
    int send(const char* buffer, size_t size, int flags = 0);
    int send_to(MemoryBuffer& buffer, SocketInfo& info, int flags = 0);
    int send_to(
        const char* buffer,
        size_t size,
        void* sock_addr,
        int sock_addr_len,
        int flags = 0
    );

    void close();
    int shutdown(int mode = -1);

    int get_fd() const;
    int detach();

    SocketInfo get_bound_address() const;
    SocketInfo get_peer_address() const;

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

    void from(void* sock_addr);
    void to(void* sock_addr, int* sock_addr_len);
    std::string str() const;

    bool operator==(const SocketInfo& other) const;
}; // struct SocketInfo

} // namespace sockspp
