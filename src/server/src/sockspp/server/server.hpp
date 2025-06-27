#pragma once

#include "server_params.hpp"
#include "session.hpp"

#include <sockspp/core/s5_enums.hpp>
#include <sockspp/core/socket.hpp>
#include <sockspp/core/poller/poller.hpp>

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

    const std::string& get_listen_ip() const;
    uint16_t get_listen_port() const;
    AuthMethod get_auth_method() const;

    bool authenticate(
        const std::string& username,
        const std::string& password
    ) const;

private:
    Socket _accept_client();
    Session* _create_new_session(Poller& poller, Socket&& sock);
    void _delete_session(Session* session);

private:
    ServerParams _params;
    Socket _server_socket;

}; // class Server

} // namespace sockspp
