#include "s5.hpp"
#include "log.hpp"
#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netinet/in.h>
#endif

namespace sockspp
{

S5Address::S5Address(void* data)
    : _data(data) {}

size_t S5Address::get_size() const
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

    LOGE("S5Address type %d", (int)type);

    return 0;
}

AddrType S5Address::get_type() const
{
    return *reinterpret_cast<AddrType*>(_data);
}

void S5Address::set_type(AddrType type)
{
    *reinterpret_cast<AddrType*>(_data) = type;
}

uint8_t* S5Address::get_address() const
{
    return reinterpret_cast<uint8_t*>(_data) + 1;
}

void S5Address::set_address(uint8_t* address)
{
    uint8_t* this_address = this->get_address();

    switch (this->get_type())
    {
    case AddrType::IPv4:
        *reinterpret_cast<uint32_t*>(this_address) = \
            *reinterpret_cast<uint32_t*>(address);
        break;
    case AddrType::IPv6:
        memcpy(this_address, address, 16);
        break;
    case AddrType::DomainName:
        {
            uint16_t port = this->get_port();
            memcpy(this_address, address, address[0] + 1);
            this->set_port(port);
        }
        break;
    default:
        break;
    }
}

uint16_t S5Address::get_port() const
{
    size_t size = this->get_size();

    if (!size)
        return 0;

    uint8_t* port_ptr = (reinterpret_cast<uint8_t*>(_data) + size - 2);

    return ntohs(*reinterpret_cast<uint16_t*>(port_ptr));
}

void S5Address::set_port(uint16_t port, bool netport)
{
    size_t size = this->get_size();

    if (!size)
        return;

    uint8_t* port_ptr = (reinterpret_cast<uint8_t*>(_data) + size - 2);

    *reinterpret_cast<uint16_t*>(port_ptr) = netport ? port : htons(port);
}

S5R_Base::S5R_Base(void* data)
    : _data(data) {}

size_t S5R_Base::get_size() const
{
    return this->get_address().get_size() + 3;
}

S5Address S5R_Base::get_address() const
{
    S5Address address(reinterpret_cast<uint8_t*>(_data) + 3);
    return address;
}

void S5R_Base::set_version(uint8_t version)
{
    *reinterpret_cast<uint8_t*>(_data) = version;
}

uint8_t S5R_Base::_get_cmd_or_rep() const
{
    return reinterpret_cast<uint8_t*>(_data)[1];
}

void S5R_Base::_set_cmd_or_rep(uint8_t cmd_or_rep)
{
    reinterpret_cast<uint8_t*>(_data)[1] = cmd_or_rep;
}

}; // namespace sockspp
