#pragma once

#include <string>
#include <cstdint>

namespace sockspp::server
{

struct ServerParams
{
    std::string listen_ip;
    uint16_t listen_port = 1080;
    std::string username;
    std::string password;
    std::string dns_ip;
    uint16_t dns_port = 53;
    bool client_tcp_nodelay = false;
    bool client_tcp_keepalive = false;
    bool remote_tcp_nodelay = false;
    bool remote_tcp_keepalive = false;
}; // class ServerParams

} // namespace sockspp::server
