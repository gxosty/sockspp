#pragma once

#include <string>
#include <cstdint>

namespace sockspp
{

struct ServerParams
{
    std::string listen_ip;
    uint16_t listen_port;
    std::string username;
    std::string password;
}; // class ServerParams

} // namespace sockspp
