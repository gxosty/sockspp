#pragma once

#include "session_socket.hpp"
#include <sockspp/core/socket.hpp>
#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/ip_address.hpp>

#include <vector>

namespace sockspp::server
{

class Session;

class RemoteSocket : public SessionSocket
{
public:
    RemoteSocket(
        Socket&& sock,
        const std::vector<IPAddress>* addresses
    );
    ~RemoteSocket();

    bool process_event(Event::Flags event_flags) override;

    bool is_connected() const;
    bool try_connect_next();
    bool could_connect();
    const SocketInfo& get_remote_info() const;

private:
    SocketInfo _remote_info;
    const std::vector<IPAddress>* _addresses;
    size_t _connecting_idx;
    bool _connected;
}; // class RemoteSocket

} // namespace sockspp::server
