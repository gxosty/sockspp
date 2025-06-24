#pragma once

#include "client_socket.hpp"
#include "remote_socket.hpp"
#include "sockspp/core/memory_buffer.hpp"

#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/socket.hpp>
#include <sockspp/core/ip_address.hpp>
#include <sockspp/core/s5_enums.hpp>

#include <vector>

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
        ResolvingDomainName,
        ConnectingRemote,
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
    bool reply_remote_connection(
        Reply reply,
        AddrType addr_type,
        uint8_t* address,
        uint16_t port
    );

private:
    void _set_state(State state);

    bool _check_version(MemoryBuffer& buffer);
    bool _request_auth(MemoryBuffer& buffer);
    bool _handle_auth(MemoryBuffer& buffer);
    bool _check_command(MemoryBuffer& buffer);
    std::vector<IPAddress>* _resolve_address(MemoryBuffer& buffer, bool* is_domain_name);
    bool _do_command(const std::vector<IPAddress>* addresses = nullptr);

    bool _connect_remote(
        Socket&& sock,
        const std::vector<IPAddress>* addresses
    );

    void _remote_connected();

private:
    const Server& _server;
    Poller& _poller;
    ClientSocket* _client_socket = nullptr;
    RemoteSocket* _remote_socket = nullptr;
    // UDPSocket _udp_socket;
    
    State _state = State::Invalid;
    Command _command = Command::Invalid;
}; // class Session

} // namespace sockspp
