#include "sockspp/core/memory_buffer.hpp"
#include <cstdint>
#include <sockspp/core/socket.hpp>
#include <sockspp/core/exceptions.hpp>
#include <stdexcept>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace sockspp
{

static void _make_address(const std::string& ip, uint16_t port, sockaddr_storage& addr)
{
    addr.ss_family = AF_INET;

    if (inet_pton(AF_INET, ip.c_str(), &((sockaddr_in*)&addr)->sin_addr))
    {
        ((sockaddr_in*)&addr)->sin_port = htons(port);
    }
    else
    {
        addr.ss_family = AF_INET6;
        inet_pton(AF_INET6, ip.c_str(), &((sockaddr_in6*)&addr)->sin6_addr);
        ((sockaddr_in6*)&addr)->sin6_port = htons(port);
    }
}

Socket::Socket(int domain, int type, int protocol)
{
#ifdef _WIN32
    _fd = WSASocket(domain, type, protocol, NULL, 0, 0);
#else
    _fd = ::socket(domain, type, protocol);
#endif

    if (_fd == -1)
    {
        throw SocketCreationException();
    }
}

Socket::Socket(int fd)
{
    _fd = fd;
}

Socket::Socket(Socket&& other)
{
    _fd = other._fd;
    other._fd = -1;
}

Socket::~Socket()
{
    this->close();
}

Socket Socket::open_tcp()
{
    return Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

Socket Socket::open_tcp6()
{
    return Socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
}

Socket Socket::open_udp()
{
    return Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

Socket Socket::open_udp6()
{
    return Socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
}

bool Socket::set_blocking(bool enabled)
{
#if (_WIN32)
    unsigned long block = !enabled;
    return ioctlsocket(_fd, FIONBIO, &block);
#elif __has_include(<sys/ioctl.h>) && defined(FIONBIO)
    unsigned int block = !enabled;
    return ioctl(_fd, FIONBIO, &block);
#else
    int delay_flag, new_delay_flag;
    delay_flag = fcntl(_fd, F_GETFL, 0);

    if (delay_flag == -1)
        return false;

    new_delay_flag = enabled ? (delay_flag & ~O_NONBLOCK) : (delay_flag | O_NONBLOCK);

    if (new_delay_flag != delay_flag)
        return !fcntl(_fd, F_SETFL, new_delay_flag);

    return false;
#endif
}

bool Socket::set_nodelay(bool enabled)
{
    int state = enabled ? 1 : 0;
    return 0 == setsockopt(
        _fd,
        IPPROTO_TCP,
        TCP_NODELAY,
        reinterpret_cast<const char*>(&state),
        sizeof(state)
    );
}

bool Socket::set_keepalive(bool enabled)
{
    int state = enabled ? 1 : 0;
    return 0 == setsockopt(
        _fd,
        IPPROTO_TCP,
        TCP_KEEPALIVE,
        reinterpret_cast<const char*>(&state),
        sizeof(state)
    );
}

void Socket::connect(const std::string& ip, uint16_t port)
{
    sockaddr_storage addr;

    _make_address(ip, port, addr);

    if (::connect(_fd, (sockaddr*)&addr, addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) != 0)
    {
        throw SocketConnectionException();
    }
}

int Socket::connect(void* sock_addr, int sock_addr_len)
{
    return ::connect(_fd, (sockaddr*)sock_addr, sock_addr_len);
}

void Socket::bind(const std::string& ip, uint16_t port)
{
    sockaddr_storage addr;

    _make_address(ip, port, addr);

    if (::bind(_fd, (sockaddr*)&addr, addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) != 0)
    {
        throw SocketBindException();
    }
}

int Socket::bind(void* sock_addr, int sock_addr_len)
{
    return ::bind(_fd, (sockaddr*)sock_addr, sock_addr_len);
}

void Socket::listen(int count)
{
    ::listen(_fd, count);
}

Socket Socket::accept(SocketInfo* info)
{
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int new_socket = ::accept(_fd, reinterpret_cast<sockaddr*>(&addr), &addr_len);

    if (new_socket == -1)
    {
        throw SocketAcceptException();
    }

    if (info)
        info->from(&addr);

    return Socket(new_socket);
}

int Socket::recv(MemoryBuffer& buffer, int flags)
{
    int size = ::recv(
        _fd,
        buffer.as<char*>(),
        buffer.get_capacity(),
        flags
    );

    if (size == -1)
    {
        throw IOException();
    }

    buffer.set_size(size);

    return size;
}

int Socket::recv(char* buffer, size_t size, int flags)
{
    return ::recv(_fd, buffer, size, flags);
}

int Socket::recv_from(MemoryBuffer& buffer, SocketInfo* info, int flags)
{
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    int size = ::recvfrom(
        _fd,
        buffer.as<char*>(),
        buffer.get_size(),
        flags,
        reinterpret_cast<sockaddr*>(&addr),
        &addr_len
    );

    if (size == -1)
    {
        throw IOException();
    }

    buffer.set_size(size);

    if (info)
        info->from(&addr);

    return size;
}

int Socket::recv_from(
    char* buffer,
    size_t size,
    void* sock_addr,
    int* sock_addr_len,
    int flags
) {
    return ::recvfrom(
        _fd,
        buffer,
        size,
        flags,
        reinterpret_cast<sockaddr*>(sock_addr),
        reinterpret_cast<socklen_t*>(sock_addr_len)
    );
}

int Socket::send(MemoryBuffer& buffer, int flags)
{
    int size = ::send(
        _fd,
        buffer.as<char*>(),
        buffer.get_size(),
        flags
    );

    if (size == -1)
    {
        throw IOException();
    }

    return size;
}

int Socket::send(const char* buffer, size_t size, int flags)
{
    return ::send(_fd, buffer, size, flags);
}

int Socket::send_to(MemoryBuffer& buffer, SocketInfo& info, int flags)
{
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (info.ip_version == SocketInfo::IPv4)
    {
        addr.ss_family = AF_INET;
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&addr);
        s->sin_addr.s_addr = *reinterpret_cast<uint32_t*>(info.ip);
        s->sin_port = info.port;
    }
    else
    {
        addr.ss_family = AF_INET6;
        sockaddr_in6* s = reinterpret_cast<sockaddr_in6*>(&addr);
        s->sin6_port = info.port;
        memcpy(&s->sin6_addr, info.ip, sizeof(s->sin6_addr));
    }

    int size = ::sendto(
        _fd,
        buffer.as<char*>(),
        buffer.get_size(),
        flags,
        reinterpret_cast<sockaddr*>(&addr),
        addr_len
    );

    if (size == -1)
    {
        throw IOException();
    }

    buffer.set_size(size);

    return size;
}

int Socket::send_to(
    const char* buffer,
    size_t size,
    void* sock_addr,
    int sock_addr_len,
    int flags
) {
    return ::sendto(
        _fd,
        buffer,
        size,
        flags,
        reinterpret_cast<sockaddr*>(sock_addr),
        sock_addr_len
    );
}

void Socket::close()
{
    if (_fd != -1)
    {
#ifdef _WIN32
        ::closesocket(_fd);
#else
        ::close(_fd);
#endif
    }

    _fd = -1;
}

int Socket::shutdown(int mode)
{
    if (mode == -1)
    {
#ifdef _WIN32
        mode = SD_BOTH;        
#else
        mode = SHUT_RDWR;
#endif
    }

    return ::shutdown(_fd, mode);
}

int Socket::get_fd() const
{
    return _fd;
}

int Socket::detach()
{
    int fd = _fd;
    _fd = -1;
    return fd;
}

SocketInfo Socket::get_bound_address() const
{
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (getsockname(_fd, reinterpret_cast<sockaddr*>(&addr), &addr_len))
    {
        throw std::runtime_error("getsockname");
    }
    
    SocketInfo info;
    info.from(&addr);

    return info;
}

SocketInfo Socket::get_peer_address() const
{
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(_fd, reinterpret_cast<sockaddr*>(&addr), &addr_len))
    {
        throw std::runtime_error("getpeername");
    }
    
    SocketInfo info;
    info.from(&addr);

    return info;
}

void SocketInfo::from(void* sock_addr)
{
    sockaddr_storage* addr = reinterpret_cast<sockaddr_storage*>(sock_addr);

    if (addr->ss_family == AF_INET)
    {
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(addr);
        memcpy(this->ip, reinterpret_cast<void*>(&s->sin_addr), sizeof(s->sin_addr));
        this->port = s->sin_port;
        this->ip_version = SocketInfo::IPv4;
    }
    else if (addr->ss_family == AF_INET6)
    {
        sockaddr_in6* s = reinterpret_cast<sockaddr_in6*>(addr);
        memcpy(this->ip, reinterpret_cast<void*>(&s->sin6_addr), sizeof(s->sin6_addr));
        this->port = s->sin6_port;
        this->ip_version = SocketInfo::IPv6;
    }
}

void SocketInfo::to(void* sock_addr, int* sock_addr_len)
{
    sockaddr_storage* saddr =
        reinterpret_cast<sockaddr_storage*>(sock_addr);

    if (this->ip_version == SocketInfo::IPv4)
    {
        saddr->ss_family = AF_INET;
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(saddr);
        s->sin_addr.s_addr =
            *reinterpret_cast<uint32_t*>(this->ip);
        s->sin_port = this->port;
        *sock_addr_len = sizeof(sockaddr_in);
    }
    else
    {
        saddr->ss_family = AF_INET6;
        sockaddr_in6* s =
            reinterpret_cast<sockaddr_in6*>(saddr);
        s->sin6_port = this->port;
        memcpy(&s->sin6_addr, this->ip, sizeof(s->sin6_addr));
        *sock_addr_len = sizeof(sockaddr_in6);
    }
}

std::string SocketInfo::str() const
{
    std::string address_str;
    
    if (this->ip_version == IPv4)
    {
        char ipstr[16];
        inet_ntop(
            AF_INET,
            this->ip,
            ipstr,
            16
        );
        address_str += std::string(ipstr);
    }
    else if (this->ip_version == IPv6)
    {
        char ipstr[40];
        inet_ntop(
            AF_INET6,
            this->ip,
            ipstr,
            40
        );
        address_str += std::string(ipstr, 39);
    }

    address_str += ":" + std::to_string(ntohs(this->port));

    return address_str;
}

bool SocketInfo::operator==(const SocketInfo& other) const
{
    if (this->ip_version != other.ip_version)
        return false;

    if (this->port != other.port)
        return false;

    if (this->ip_version == SocketInfo::IPv4)
    {
        if (
            *reinterpret_cast<const uint32_t*>(this->ip)
            != *reinterpret_cast<const uint32_t*>(other.ip)
        ) return false;
    }
    else
    {
        if (memcmp(this->ip, other.ip, sizeof(in6_addr)))
            return false;
    }

    return true;
}

} // namespace sockspp
