#pragma once

#include "session_socket.hpp"
#include <sockspp/core/s5_enums.hpp>

namespace sockspp
{

class ClientSocket : public SessionSocket
{
public:
    ClientSocket(Socket&& sock);
    bool process() override;

    bool send_auth(AuthMethod method);
    bool send_auth_status(uint8_t status = 0x00);
}; // class ClientSocket

} // namespace sockspp
