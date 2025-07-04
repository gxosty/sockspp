#include "client_socket.hpp"
#include "session.hpp"
#include <sockspp/core/s5.hpp>
#include <sockspp/core/exceptions.hpp>
#include <sockspp/core/log.hpp>

namespace sockspp::server
{

ClientSocket::ClientSocket(Socket&& sock)
    : SessionSocket(std::move(sock))
{
    Socket& _sock = this->get_socket();
    _sock.set_blocking(false);
    _sock.set_nodelay(true);
}

bool ClientSocket::process_event(Event::Flags event_flags)
{
    return this->get_session().process_client_event(event_flags);
}

bool ClientSocket::send_auth(AuthMethod method)
{
    uint8_t data[2];
    data[0] = 0x05;
    data[1] = static_cast<uint8_t>(method);

    return get_socket().send(reinterpret_cast<const char*>(data), 2) != -1;
}

bool ClientSocket::send_auth_status(uint8_t status)
{
    uint8_t data[2];
    data[0] = 0x05;
    data[1] = status;

    return get_socket().send(reinterpret_cast<const char*>(data), 2) != -1;
}

bool ClientSocket::send_reply(
    Reply reply,
    AddrType addr_type,
    uint8_t* bind_addr,
    uint16_t bind_port
) {
    uint8_t data[262];
    S5ReplyMessage message(data);
    message.set_version(5);
    message.set_reply(reply);
    S5Address address = message.get_address();
    address.set_type(addr_type);
    address.set_address(bind_addr);
    address.set_port(bind_port);
    return get_socket().send(reinterpret_cast<const char*>(data), message.get_size());
}

} // namespace sockspp::server

