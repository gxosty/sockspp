#pragma once

#include "session_socket.hpp"
#include <sockspp/core/socket.hpp>
#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/ip_address.hpp>

#include <vector>

namespace sockspp
{

class Session;

class RemoteSocket : public SessionSocket
{
public:
    RemoteSocket(
        Socket&& sock,
        const std::vector<IPAddress>* addresses
    );

    bool process_event(Event::Flags event_flags) override;

private:
    bool _try_connect_next();
    bool _could_connect();

private:
    const std::vector<IPAddress>* _addresses;
    size_t _connecting_idx;
    bool _connected;
}; // class RemoteSocket

} // namespace sockspp
