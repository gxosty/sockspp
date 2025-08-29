#pragma once

#include "server_params.hpp"
#include "server_hook.hpp"
#include "session.hpp"

#include <sockspp/core/s5_enums.hpp>
#include <sockspp/core/socket.hpp>
#include <sockspp/core/ip_address.hpp>
#include <sockspp/core/poller/poller.hpp>

#include <vector>
#include <memory>

namespace sockspp::server
{

class Server
{
public:
    Server() = delete;
    Server(const ServerParams&& params);

    void serve();
    void stop();
    bool is_serving() const;

    void set_hook(std::unique_ptr<ServerHook>&& hook);
    const std::unique_ptr<ServerHook>& get_hook() const;

    const std::string& get_listen_ip() const;
    uint16_t get_listen_port() const;
    AuthMethod get_auth_method() const;
    const std::string& get_dns_ip() const;
    uint16_t get_dns_port() const;
    bool get_client_tcp_nodelay() const;
    bool get_client_tcp_keepalive() const;
    bool get_remote_tcp_nodelay() const;
    bool get_remote_tcp_keepalive() const;

    bool authenticate(
        const std::string& username,
        const std::string& password
    ) const;

private:
    Socket _accept_client();
    Session* _create_new_session(Poller& poller, Socket&& sock);
    void _delete_session(Session* session);
    void _delete_all_sessions();

private:
    ServerParams _params;
    std::unique_ptr<ServerHook> _hook;
    std::vector<Session*> _sessions;
    Socket _server_socket;

}; // class Server

} // namespace sockspp::server
