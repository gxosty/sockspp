#pragma once

#include <string>
#include <cstddef>
#include <cstdint>

namespace sockspp
{

class IPAddress
{
public:
    enum class Version : uint8_t
    {
        IPv4,
        IPv6
    };

public:
    IPAddress(const std::string& ip, uint16_t port, bool netport = false);
    IPAddress(Version version, uint8_t* ip, uint16_t port, bool netport = false);

    Version get_version() const;
    uint8_t* get_address() const;
    uint16_t get_port() const;
    uint16_t get_netport() const;  // network byte order

private:
    uint8_t _storage[16];
    uint16_t _port;
    Version _version;
    
}; // class Address

} // namespace sockspp
