#pragma once

#include "server_params.hpp"
#include <sockspp/core/socket.hpp>

namespace sockspp
{

class Server
{
public:
    Server() = delete;
    Server(const ServerParams&& params);

    void serve();
    void stop();
    bool is_serving() const;

private:
    void _accept_new_client();

private:
    ServerParams _params;
    Socket _server_socket;

}; // class Server

} // namespace sockspp
