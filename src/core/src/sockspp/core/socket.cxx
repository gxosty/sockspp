#include <sockspp/core/socket.hpp>
#include <sockspp/core/exceptions.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
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

void Socket::connect(const std::string& ip, uint16_t port)
{
    sockaddr_storage addr;

    _make_address(ip, port, addr);

    if (::connect(_fd, (sockaddr*)&addr, addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) != 0)
    {
        throw SocketConnectionException();
    }

    return 0;
}

void Socket::bind(const std::string& ip, uint16_t port)
{
    sockaddr_storage addr;

    _make_address(ip, port, addr);

    if (::bind(_fd, (sockaddr*)&addr, addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) != 0)
    {
        throw SocketBindException();
    }

    return 0;
}

int Socket::recv(Buffer& buffer, int flags)
{
    size_t size = ::recv(_fd, buffer.cstr(), buffer.get_capacity(), flags);

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

int Socket::send(Buffer& buffer, int flags)
{
    size_t size = ::send(_fd, buffer.cstr(), buffer.get_size(), flags);

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

void Socket::close()
{
    if (_fd != -1)
    {
#ifdef _WIN32
        closesocket(_fd);
#else
        ::close(_fd);
#endif
    }
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

int Socket::detach()
{
    int fd = _fd;
    _fd = -1;
    return fd;
}

} // namespace sockspp
