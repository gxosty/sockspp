#include "remote_socket.hpp"

namespace sockspp
{

RemoteSocket::RemoteSocket(Socket&& sock)
    : SessionSocket(std::move(sock)) {}

bool RemoteSocket::process()
{
    return false;
}

} // namespace sockspp
