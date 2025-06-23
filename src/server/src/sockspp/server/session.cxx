#include "session.hpp"
#include "server.hpp"
#include "sockspp/core/s5.hpp"

#include <sockspp/core/s5.hpp>
#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/poller/event.hpp>
#include <sockspp/core/log.hpp>

namespace sockspp
{

Session::Session(
    const Server& server,
    Poller& poller,
    Socket&& sock
)   : _server(server)
    , _poller(poller)
    , _client_socket(new ClientSocket(std::move(sock))) {}

Session::~Session()
{
    delete _client_socket;

    if (_remote_socket)
        delete _remote_socket;

    // if (_udp_socket)
    //     delete _udp_socket;
}

void Session::initialize()
{
    _client_socket->set_session(*this);
    _client_socket->get_socket().set_blocking(false);
    _client_socket->get_socket().set_nodelay(true);

    Event client_event(
        _client_socket->get_socket().get_fd(),
        static_cast<Event::Flags>(Event::Read | Event::Closed),
        _client_socket
    );

    _poller.register_event(client_event);

    _state = Session::State::Accepted;

    LOGD("Session initialized");
}

void Session::shutdown()
{
    Event client_event(
        _client_socket->get_socket().get_fd(),
        Event::Read,
        nullptr
    );

    _poller.remove_event(client_event);
    _client_socket->get_socket().shutdown();
    _client_socket->get_socket().close();

    if (_remote_socket)
    {
        Event remote_event(
            _remote_socket->get_socket().get_fd(),
            Event::Read,
            nullptr
        );

        _poller.remove_event(remote_event);
        _remote_socket->get_socket().shutdown();
        _remote_socket->get_socket().close();
    }

    // if (_udp_socket)
    // {
    //     Event udp_event(
    //         _udp_socket->get_socket().get_fd(),
    //         Event::Read,
    //         nullptr
    //     );
    //
    //     _poller.remove_event(udp_event);
    //     _udp_socket->get_socket().shutdown();
    //     _udp_socket->get_socket().close();
    // }
    
    LOGD("Session shutdown");
}

bool Session::process_client(MemoryBuffer& buffer)
{
    switch (_state)
    {
    case Session::State::Accepted:
        return _check_version(buffer) && _request_auth(buffer);
    case Session::State::AuthRequested:
        return _handle_auth(buffer);
    case Session::State::Authenticated:
        return _handle_command(buffer);
    default:
        break;
    }

    LOGE("Undefined session state");
    return false;
}

bool Session::process_remote(MemoryBuffer& buffer)
{
    return false;
}

void Session::_set_state(Session::State state)
{
    _state = state;
}

bool Session::_check_version(MemoryBuffer& buffer)
{
    uint8_t* data = buffer.as<uint8_t*>();
    if (data[0] == 5)
    {
        return true;
    }

    _client_socket->send_auth(AuthMethod::Invalid);
    LOGI("Client invalid version specified: %d", (int)data[0]);

    return false;
}

bool Session::_request_auth(MemoryBuffer& buffer)
{
    uint8_t* data = buffer.as<uint8_t*>();
    AuthMethod auth_method = _server.get_auth_method();
    AuthMethod selected_method = AuthMethod::Invalid;

    for (uint8_t i = 0; i < data[1]; i++)
    {
        AuthMethod method = static_cast<AuthMethod>(data[i + 2]);

        if (method == auth_method)
        {
            selected_method = method;
            break;
        }
    }

    if (selected_method != auth_method)
    {
        LOGI("Invalid client authentication method");
        return false;
    }

    if (!_client_socket->send_auth(selected_method))
    {
        LOGE("Error occured when sending auth method");
        return false;
    }

    if (selected_method == AuthMethod::NoAuth)
    {
        _set_state(Session::State::Authenticated);
    }
    else if (selected_method == AuthMethod::UserPass)
    {
        _set_state(Session::State::AuthRequested);
    }

    return true;
}

bool Session::_handle_auth(MemoryBuffer& buffer)
{
    uint8_t* data = buffer.as<uint8_t*>();
    size_t data_size = buffer.get_size();

    data += 1; // skip version
    uint8_t username_len = data[0];

    if (username_len > (data_size - 2))
    {
        // someone must be trying to play a segfault game with us
        LOGE("Username length is longer than the buffer itself");
        return false;
    }

    data += 1 + username_len; // skip username and length
    uint8_t password_len = data[0];

    if (password_len > (data_size - username_len - 3))
    {
        // the same story
        LOGE("Password length is longer than the buffer itself");
        return false;
    }

    std::string username;
    std::string password;

    if (username_len)
        username = std::string(buffer.as<char*>() + 2, username_len);

    if (password_len)
        password = std::string(buffer.as<char*>() + 3 + username_len, password_len);

    if (!_server.authenticate(username, password))
    {
        _client_socket->send_auth_status(0xFF);
        LOGI("Wrong client username or password");
        return false;
    }

    _client_socket->send_auth_status(0x00);
    _set_state(Session::State::Authenticated);
    LOGI("Client authentication succeeded");
    return true;
}

bool Session::_handle_command(
    MemoryBuffer& buffer,
    const std::vector<IPAddress>* resolved_addresses
) {
    uint8_t* data = buffer.as<uint8_t*>();
    S5CommandMessage message(data);
    S5Address address = message.get_address();

    Command command = message.get_command();
    _command = command;
    Reply reply = Reply::Invalid;

    switch (command)
    {
    case Command::Connect:
        break;
    case Command::UdpAssociate:
    case Command::Bind:
    default:
        LOGE("Unsupported command");
        reply = Reply::CommandNotSupported;
    }

    if (reply != Reply::Invalid)
    {
        _client_socket->send_reply(
            reply,
            address.get_type(),
            address.get_address(),
            address.get_port()
        );

        return false;
    }

    return true;
}

bool Session::_connect_remote(
    Socket&& sock,
    const std::vector<IPAddress>* addresses
) {
    sock.set_blocking(false);
    sock.set_nodelay(true);

    _remote_socket = new RemoteSocket(std::move(sock));

    return false;
}

} // namespace sockspp
