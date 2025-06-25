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

namespace sockspp
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
}

bool RemoteSocket::process_event(Event::Flags event_flags)
{
    if (event_flags & Event::Closed)
    {
        return false;
    }

    if (event_flags & Event::Error)
    {
        if (!_connected)
        {
            return _try_connect_next();
        }

        return false;
    }

    if (event_flags & Event::Write)
    {
        return _could_connect();
    }

    uint8_t _buffer[4096];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        4096
    );

    int status = this->recv(buffer);

    if (status == 0)
    {
        return false;
    }
    else if (status == -1)
    {
        return false;
    }

    return this->get_session().process_remote(buffer);
}

bool RemoteSocket::_try_connect_next()
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

        if (res < 0
#ifdef _WIN32
            && (sockerrno != WSAEWOULDBLOCK)
#else
            && (sockerrno != EWOULDBLOCK)
#endif // _WIN32
        ) {
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

bool RemoteSocket::_could_connect()
{
    _connected = true;
    IPAddress connected_address = _addresses->at(_connecting_idx);
    delete _addresses;
    _addresses = nullptr;

    return this->get_session().reply_remote_connection(
        Reply::Success,
        connected_address.get_version() == IPAddress::Version::IPv4
            ? AddrType::IPv4
            : AddrType::IPv6,
        connected_address.get_address(),
        connected_address.get_port()
    );
}

} // namespace sockspp
