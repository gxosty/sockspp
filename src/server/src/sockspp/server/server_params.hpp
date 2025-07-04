#pragma once

#include <string>
#include <cstdint>

namespace sockspp
{

struct ServerParams
{
    std::string listen_ip;
    uint16_t listen_port = 1080;
    std::string username;
    std::string password;
    std::string dns_ip;
    uint16_t dns_port = 53;
}; // class ServerParams

} // namespace sockspp
