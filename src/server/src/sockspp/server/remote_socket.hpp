#pragma once

#include "session_socket.hpp"
#include <sockspp/core/socket.hpp>
#include <sockspp/core/poller/poller.hpp>

namespace sockspp
{

class Session;

class RemoteSocket : public SessionSocket
{
public:
    RemoteSocket(Socket&& sock);
    bool process() override;

}; // class RemoteSocket

} // namespace sockspp
