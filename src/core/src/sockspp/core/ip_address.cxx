#include "ip_address.hpp"
#include <stdexcept>
#include <cstdlib>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#endif // _WIN32

namespace sockspp
{

IPAddress::IPAddress(const std::string& ip, uint16_t port, bool netport)
{
    if (inet_pton(AF_INET, ip.c_str(), this->get_address()))
    {
        _version = IPAddress::Version::IPv4;
    }
    else if (inet_pton(AF_INET6, ip.c_str(), this->get_address()))
    {
        _version = IPAddress::Version::IPv6;
    }
    else
    {
        throw std::runtime_error("Invalid IP address: " + ip);
    }

    if (netport)
    {
        _port = ntohs(port);
    }
    else
    {
        _port = port;
    }
}

IPAddress::IPAddress(Version version, uint8_t* ip, uint16_t port, bool netport)
{
    if (version == IPAddress::Version::IPv4)
    {
        *reinterpret_cast<uint32_t*>(_storage) = \
            *reinterpret_cast<uint32_t*>(ip);
    }
    else
    {
        memcpy(_storage, ip, 16);
    }

    _version = version;

    if (netport)
    {
        _port = ntohs(port);
    }
    else
    {
        _port = port;
    }
}

IPAddress::Version IPAddress::get_version() const
{
    return _version;
}

uint8_t* IPAddress::get_address() const
{
    return const_cast<uint8_t*>(_storage);
}

uint16_t IPAddress::get_port() const
{
    return _port;
}

uint16_t IPAddress::get_netport() const
{
    return htons(_port);
}

} // namespace sockspp
