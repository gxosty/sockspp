#pragma once

#include "client_socket.hpp"
#include "remote_socket.hpp"

#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/socket.hpp>

namespace sockspp
{

class Server;

class Session
{
public:
    enum class State
    {
        Accepted,
        AuthRequested,
        Authenticated,
        Connected,
        Invalid
    };

public:
    Session(
        const Server& server,
        Poller& poller,
        Socket&& sock
    );
    ~Session();

    void initialize();
    void shutdown();

    bool process_client(MemoryBuffer& buffer);
    bool process_remote(MemoryBuffer& buffer);

private:
    void _set_state(State state);

    bool _check_version(MemoryBuffer& buffer);
    bool _request_auth(MemoryBuffer& buffer);
    bool _handle_auth(MemoryBuffer& buffer);

private:
    const Server& _server;
    Poller& _poller;
    ClientSocket* _client_socket = nullptr;
    RemoteSocket* _remote_socket = nullptr;
    // UDPSocket _udp_socket;
    
    State _state = State::Invalid;
}; // class Session

} // namespace sockspp
