#include "udp_socket.hpp"
#include "session.hpp"
#include "defs.hpp"

#include <sockspp/core/s5.hpp>
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

UDPSocket::UDPSocket(Socket&& sock, SocketInfo client_info)
    : SessionSocket(std::move(sock)), _client_info(client_info)
{
    Socket& _sock = this->get_socket();
    _sock.set_blocking(false);
    _sock.set_nodelay(true);
}

bool UDPSocket::process_event(Event::Flags event_flags)
{
    return this->get_session().process_udp_event(event_flags);
}

int UDPSocket::recv_from(
    MemoryBuffer& buffer,
    void* addr,
    int* addr_len
) {
    int res = SessionSocket::recv_from(buffer, addr, addr_len);

    if (res <= 10)
    {
        LOGD("res <= 10");
        return res;
    }

    SocketInfo info;
    info.from(addr);

    uint16_t port_bkp = _client_info.port;
    _client_info.port = info.port;
    bool is_client = info == _client_info;
    _client_info.port = port_bkp;

    if (!is_client)
    {
        // drop packet for invalid source ip
        LOGD("!is_client");
        return 0;
    }

    if (!addr || !addr_len)
        return res;

    S5UDPHeader header(buffer.as<uint8_t*>());
    S5Address remote_address = header.get_address();
    AddrType remote_address_type = remote_address.get_type();

    if (remote_address_type == AddrType::DomainName)
    {
        // sorry, we don't support it for udp... yet
        LOGD("remote_address_type == AddrType::DomainName");
        return 0;
    }

    sockaddr_storage remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);

    if (remote_address_type == AddrType::IPv4)
    {
        remote_addr.ss_family = AF_INET;
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&remote_addr);
        s->sin_addr.s_addr =
            *reinterpret_cast<uint32_t*>(remote_address.get_address());
        s->sin_port = htons(remote_address.get_port());
    }
    else if (remote_address_type == AddrType::IPv6)
    {
        remote_addr.ss_family = AF_INET6;
        sockaddr_in6* s =
            reinterpret_cast<sockaddr_in6*>(&remote_addr);
        s->sin6_port = htons(remote_address.get_port());
        memcpy(&s->sin6_addr, remote_address.get_address(), sizeof(s->sin6_addr));
    }
    else
    {
        // we don't support other address types too
        return 0;
    }

    int new_sock_addr_len = 
        *addr_len < remote_addr_len
            ? *addr_len
            : remote_addr_len;

    memcpy(
        addr,
        &remote_addr,
        new_sock_addr_len
    );

    *addr_len = new_sock_addr_len;
    _port_maps[htons(remote_address.get_port())] = info.port;

    int header_size = header.get_size();
    int buffer_size = buffer.get_size() - header_size;
    memcpy(buffer.get_ptr(), buffer.as<uint8_t*>() + header_size, buffer_size);
    buffer.set_size(buffer_size);

    LOG_SCOPE(LogLevel::Debug)
    {
        SocketInfo remote_info;
        remote_info.from(&remote_addr);

        LOGD("UDP | %s -> %s | %zu",
            info.str().c_str(),
            remote_info.str().c_str(),
            buffer.get_size()
        );
    }

    return buffer_size;
}

int UDPSocket::send_to(
    MemoryBuffer& buffer,
    void* addr,
    int addr_len
) {
    SocketInfo remote_info;
    remote_info.from(addr);
    if (!_port_maps.contains(remote_info.port))
    {
        LOGE("UDP Remote port wasn't mapped");
        return 0;
    }

    // prepare header
    uint8_t _header[SOCKSPP_SESSION_SOCKET_BUFFER_SIZE + 22];
    _header[0] = 0;
    _header[1] = 0;
    _header[2] = 0;
    S5UDPHeader header(_header);
    S5Address address = header.get_address();
    address.set_type(
        remote_info.ip_version == SocketInfo::IPv4
        ? AddrType::IPv4
        : AddrType::IPv6
    );
    address.set_address(remote_info.ip);
    address.set_port(remote_info.port, true);

    memcpy(
        reinterpret_cast<uint8_t*>(_header) + header.get_size(),
        buffer.as<uint8_t*>(),
        buffer.get_size()
    );

    MemoryBuffer send_buffer(
        _header,
        header.get_size() + buffer.get_size(),
        SOCKSPP_SESSION_SOCKET_BUFFER_SIZE + 22
    );

    sockaddr_storage send_addr;
    socklen_t send_addr_len;

    if (_client_info.ip_version == SocketInfo::IPv4)
    {
        send_addr.ss_family = AF_INET;
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&send_addr);
        s->sin_addr.s_addr =
            *reinterpret_cast<uint32_t*>(_client_info.ip);
        s->sin_port = _port_maps[remote_info.port];
        send_addr_len = sizeof(sockaddr_in);
    }
    else
    {
        send_addr.ss_family = AF_INET6;
        sockaddr_in6* s =
            reinterpret_cast<sockaddr_in6*>(&send_addr);
        s->sin6_port = _port_maps[remote_info.port];
        memcpy(&s->sin6_addr, _client_info.ip, sizeof(s->sin6_addr));
        send_addr_len = sizeof(sockaddr_in6);
    }

    int res = SessionSocket::send_to(send_buffer, &send_addr, send_addr_len);

    if (res <= 0)
    {
        LOGE("UDP send_to res: %d", res);
        return res;
    }

    LOG_SCOPE(LogLevel::Debug)
    {
        SocketInfo client_info;
        client_info.from(&send_addr);

        LOGD("UDP | %s <- %s | %zu",
            client_info.str().c_str(),
            remote_info.str().c_str(),
            send_buffer.get_size()
        );
    }

    return buffer.get_size();
}

} // namespace sockspp::server
