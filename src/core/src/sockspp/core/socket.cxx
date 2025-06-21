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

void Socket::connect(const std::string& ip, uint16_t port)
{
    sockaddr_storage addr;

    _make_address(ip, port, addr);

    if (::connect(_fd, (sockaddr*)&addr, addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) != 0)
    {
        throw SocketConnectionException();
    }
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
    {
        if (addr.ss_family == AF_INET)
        {
            sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&addr);
            memcpy(info->ip, reinterpret_cast<void*>(&s->sin_addr), sizeof(s->sin_addr));
            info->port = ntohs(s->sin_port);
            info->ip_version = SocketInfo::IPv4;
        }
        else if (addr.ss_family == AF_INET6)
        {
            sockaddr_in6* s = reinterpret_cast<sockaddr_in6*>(&addr);
            memcpy(info->ip, reinterpret_cast<void*>(&s->sin6_addr), sizeof(s->sin6_addr));
            info->port = ntohs(s->sin6_port);
            info->ip_version = SocketInfo::IPv6;
        }
    }

    return Socket(new_socket);
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

} // namespace sockspp
