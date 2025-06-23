#include "client_socket.hpp"
#include "session.hpp"
#include <sockspp/core/s5.hpp>
#include <sockspp/core/exceptions.hpp>
#include <sockspp/core/log.hpp>

namespace sockspp
{

ClientSocket::ClientSocket(Socket&& sock)
    : SessionSocket(std::move(sock)) {}

bool ClientSocket::process()
{
    uint8_t _buffer[4096];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        4096
    );

    Socket& socket = this->get_socket();
    bool status = false;

    try {
        socket.recv(buffer);
        status = buffer.get_size() > 0;
    } catch (const IOException& ex) {
        LOGE("%s", ex.what());
    }

#if !SOCKSPP_DISABLE_LOGS
    if (buffer.get_size() == 0)
        LOGI("Client closed connection");
#endif

    if (status)
    {
        return get_session().process_client(buffer);
    }

    return false;
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

} // namespace sockspp

