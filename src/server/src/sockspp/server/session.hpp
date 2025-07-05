#pragma once

#include "client_socket.hpp"
#include "remote_socket.hpp"
#include "udp_socket.hpp"
#include "dns_socket.hpp"

#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/buffer.hpp>
#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/socket.hpp>
#include <sockspp/core/ip_address.hpp>
#include <sockspp/core/s5_enums.hpp>

#include <vector>

namespace sockspp::server
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
        Associated,
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

    bool process_client_event(Event::Flags event_flags);
    bool process_remote_event(Event::Flags event_flags);
    bool process_udp_event(Event::Flags event_flags);

    bool reply_remote_connection(
        Reply reply,
        AddrType addr_type,
        uint8_t* address,
        uint16_t port
    );

private:
    void _set_state(State state);

    bool _process_client(MemoryBuffer& buffer, void* addr, int addr_len);
    bool _process_remote(MemoryBuffer& buffer, void* addr, int addr_len);
    bool _session_socket_send(
        SessionSocket* session_socket,
        MemoryBuffer* buffer,
        MemoryBuffer& _scheduled
    );

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
    bool _associate(Socket&& cl_sock, Socket&& rm_sock);

private:
    Buffer _client_buffer;
    Buffer _remote_buffer;
    SocketInfo _peer_info;
    const Server& _server;
    Poller& _poller;
    ClientSocket* _client_socket;
    RemoteSocket* _remote_socket;
    UDPSocket* _udp_socket;
    
    State _state = State::Invalid;
    Command _command = Command::Invalid;
}; // class Session

} // namespace sockspp::server
