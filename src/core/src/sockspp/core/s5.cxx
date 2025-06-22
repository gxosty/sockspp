#include "s5.hpp"
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netinet/in.h>
#endif

namespace sockspp
{

Address::Address(void* data)
    : _data(data) {}

size_t Address::get_size() const
{
    AddrType type = this->get_type();

    switch (type)
    {
    case AddrType::IPv4:
        return 7;
    case AddrType::IPv6:
        return 19;
    case AddrType::DomainName:
        {
            uint8_t* address = reinterpret_cast<uint8_t*>(this->get_address());
            return address[0] + 4;
        }
    default:
        break;
    }

    return 0;
}

AddrType Address::get_type() const
{
    return *reinterpret_cast<AddrType*>(_data);
}

void Address::set_type(AddrType type)
{
    *reinterpret_cast<AddrType*>(_data) = type;
}

uint8_t* Address::get_address() const
{
    return reinterpret_cast<uint8_t*>(_data) + 1;
}

uint16_t Address::get_port() const
{
    size_t size = this->get_size();

    if (!size)
        return 0;

    uint8_t* port_ptr = (reinterpret_cast<uint8_t*>(_data) + size - 2);

    return ntohs(*reinterpret_cast<uint16_t*>(port_ptr));
}

void Address::set_port(uint16_t port)
{
    size_t size = this->get_size();

    if (!size)
        return;

    uint8_t* port_ptr = (reinterpret_cast<uint8_t*>(_data) + size - 2);

    *reinterpret_cast<uint16_t*>(port_ptr) = htons(port);
}

R_Base::R_Base(void* data)
    : _data(data) {}

size_t R_Base::get_size() const
{
    return this->get_address().get_size() + 3;
}

Address R_Base::get_address() const
{
    Address address(reinterpret_cast<uint8_t*>(_data) + 4);
    return address;
}

uint8_t R_Base::_get_cmd_or_rep() const
{
    return reinterpret_cast<uint8_t*>(_data)[1];
}

}; // namespace sockspp
