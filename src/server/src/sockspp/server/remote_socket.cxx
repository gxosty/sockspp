#include "remote_socket.hpp"
#include "session.hpp"
#include "sockspp/core/ip_address.hpp"

#include <sockspp/core/errno.hpp>
#include <sockspp/core/log.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

namespace sockspp::server
{

RemoteSocket::RemoteSocket(
    Socket&& sock,
    const std::vector<IPAddress>* addresses
)   : SessionSocket(std::move(sock))
    , _addresses(addresses)
    , _connecting_idx(-1)
    , _connected(false)
{
    Socket& _sock = this->get_socket();
    _sock.set_blocking(false);
    _sock.set_nodelay(true);

    if (!addresses)
        _connected = true;
}

RemoteSocket::~RemoteSocket()
{
    if (_addresses)
        delete _addresses;
}

bool RemoteSocket::process_event(Event::Flags event_flags)
{
    return this->get_session().process_remote_event(event_flags);
}

bool RemoteSocket::is_connected() const
{
    return _connected;
}

bool RemoteSocket::try_connect_next()
{
    _connecting_idx++;

    if (_connecting_idx < _addresses->size())
    {
        const IPAddress& addr = _addresses->at(_connecting_idx);
        IPAddress::Version addr_ver = addr.get_version();
        sockaddr_storage sock_addr;

        if (addr_ver == IPAddress::Version::IPv4)
        {
            sock_addr.ss_family = AF_INET;
            sockaddr_in* _sock_addr = reinterpret_cast<sockaddr_in*>(&sock_addr);
            _sock_addr->sin_addr.s_addr =
                *reinterpret_cast<uint32_t*>(addr.get_address());
            _sock_addr->sin_port = addr.get_netport();
        }
        else
        {
            sock_addr.ss_family = AF_INET6;
            sockaddr_in6* _sock_addr = reinterpret_cast<sockaddr_in6*>(&sock_addr);
            memcpy(
                &_sock_addr->sin6_addr,
                addr.get_address(),
                16
            );
            _sock_addr->sin6_port = addr.get_netport();
        }

        int res = this->get_socket().connect(
            reinterpret_cast<sockaddr*>(&sock_addr),
            addr_ver == IPAddress::Version::IPv4
                ? sizeof(sockaddr_in)
                : sizeof(sockaddr_in6)
        );

        if (res < 0 && (sockerrno == EWOULDBLOCK)) {
            // TODO: Reply based on errno and fix reply address
            this->get_session().reply_remote_connection(
                Reply::GeneralFailure,
                addr_ver == IPAddress::Version::IPv4
                    ? AddrType::IPv4
                    : AddrType::IPv6,
                addr.get_address(),
                addr.get_port()
            );
            return false;
        }

        return true;
    }

    return false;
}

bool RemoteSocket::could_connect()
{
    _connected = true;
    IPAddress connected_address = _addresses->at(_connecting_idx);
    delete _addresses;
    _addresses = nullptr;

    _remote_info = this->get_socket().get_peer_address();

    return this->get_session().reply_remote_connection(
        Reply::Success,
        connected_address.get_version() == IPAddress::Version::IPv4
            ? AddrType::IPv4
            : AddrType::IPv6,
        connected_address.get_address(),
        connected_address.get_port()
    );
}

const SocketInfo& RemoteSocket::get_remote_info() const
{
    return _remote_info;
}

} // namespace sockspp::server
