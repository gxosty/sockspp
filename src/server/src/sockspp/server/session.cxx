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

    Event client_event(
        _client_socket->get_socket().get_fd(),
        static_cast<Event::Flags>(Event::Read | Event::Closed),
        _client_socket
    );

    _poller.register_event(client_event);

    _state = Session::State::Accepted;
}

void Session::shutdown()
{
    Event client_event(
        _client_socket->get_socket().get_fd(),
        static_cast<Event::Flags>(Event::Read | Event::Closed),
        nullptr
    );

    _poller.remove_event(client_event);
    _client_socket->get_socket().shutdown();
    _client_socket->get_socket().close();

    if (_remote_socket)
    {
        Event::Flags flags = static_cast<Event::Flags>(Event::Closed | (
            _state == Session::State::ConnectingRemote
            ? Event::Write
            : Event::Read
        ));

        Event remote_event(
            _remote_socket->get_socket().get_fd(),
            flags,
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
}

bool Session::process_client_event(Event::Flags event_flags)
{
    if (event_flags & (Event::Closed | Event::Error))
    {
        return false;
    }

    uint8_t _buffer[4096];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        4096
    );

    int status = _client_socket->recv(buffer);

    if (status == 0)
    {
        return false;
    }
    else if (status == -1)
    {
        LOGE("Client error: %d", sockerrno);
        return false;
    }

    return _process_client(buffer);
}

bool Session::process_remote_event(Event::Flags event_flags)
{
    if (event_flags & Event::Closed)
    {
        return false;
    }

    RemoteSocket* remote_socket =
        reinterpret_cast<RemoteSocket*>(_remote_socket);

    if (event_flags & Event::Error)
    {
        if (!remote_socket->is_connected())
        {
            return remote_socket->try_connect_next();
        }

        return false;
    }

    if (event_flags & Event::Write)
    {
        if (!remote_socket->is_connected())
            return remote_socket->could_connect();
    }

    uint8_t _buffer[4096];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        4096
    );

    int status = remote_socket->recv(buffer);

    if (status == 0)
    {
        return false;
    }
    else if (status == -1)
    {
        return false;
    }

    return _process_remote(buffer);
}

bool Session::_process_client(MemoryBuffer& buffer)
{
    switch (_state)
    {
    case Session::State::Accepted:
        return _check_version(buffer) && _request_auth(buffer);
    case Session::State::AuthRequested:
        return _handle_auth(buffer);
    case Session::State::Authenticated:
        {
            if (!_check_command(buffer))
                return false;

            bool is_domain_name = false;
            std::vector<IPAddress>* addresses = _resolve_address(buffer, &is_domain_name);

            if (is_domain_name)
                return true;

            if (!addresses)
                return false;

            return _do_command(addresses);
        }
    case Session::State::Connected:
        return _remote_socket->send(buffer);
    default:
        break;
    }

    LOGE("Undefined session state");
    return false;
}

bool Session::reply_remote_connection(
    Reply reply,
    AddrType addr_type,
    uint8_t* address,
    uint16_t port
) {
    if (_state != Session::State::ConnectingRemote)
    {
        LOGE("Remote connected in wrong session state");
        return false;
    }
        
    if (reply == Reply::Success)
    {
        _remote_connected();
    }

    return _client_socket->send_reply(reply, addr_type, address, port)
        && (reply == Reply::Success);
}

bool Session::_process_remote(MemoryBuffer& buffer)
{
    switch (_state)
    {
    case Session::State::Connected:
        return _client_socket->send(buffer);
    default:
        break;
    }

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

    if (!_client_socket->send_auth(selected_method))
    {
        return false;
    }

    if (selected_method != auth_method)
    {
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
        return false;
    }

    _client_socket->send_auth_status(0x00);
    _set_state(Session::State::Authenticated);
    return true;
}

bool Session::_check_command(MemoryBuffer& buffer)
{
    S5CommandMessage message(buffer.as<uint8_t*>());
    Command command = message.get_command();
    _command = command;

    switch (command)
    {
    case Command::Connect:
        break;
    case Command::Bind:
    case Command::UdpAssociate:
    default:
        LOGW("Unsupported command: %d", static_cast<int>(command));
        {
            S5Address address = message.get_address();
            _client_socket->send_reply(
                Reply::CommandNotSupported,
                address.get_type(),
                address.get_address(),
                address.get_port()
            );
        }
        return false;
    }

    return true;
}

std::vector<IPAddress>* Session::_resolve_address(MemoryBuffer& buffer, bool* is_domain_name)
{
    S5CommandMessage message(buffer.as<uint8_t*>());
    S5Address address = message.get_address();
    *is_domain_name = false;
    AddrType type = address.get_type();

    switch (type)
    {
    case AddrType::IPv4:
    case AddrType::IPv6:
        {
            std::vector<IPAddress>* addresses = new std::vector<IPAddress>();

            addresses->emplace_back(
                type == AddrType::IPv4
                ? IPAddress::Version::IPv4
                : IPAddress::Version::IPv6,
                address.get_address(),
                address.get_port(),
                false
            );

            return addresses;
        }
    case AddrType::DomainName:
        // *is_domain_name = true;
    default:
        LOGW("Unsupported address type: %d", static_cast<int>(type));
        {
            _client_socket->send_reply(
                Reply::AddrTypeNotSupported,
                type,
                address.get_address(),
                address.get_port()
            );
        }
        break;
    }

    return nullptr;
}

bool Session::_do_command(
    const std::vector<IPAddress>* addresses
) {
    switch (_command)
    {
    case Command::Connect:
        {
            Socket sock = \
                addresses->at(0).get_version() == IPAddress::Version::IPv4
                ? Socket::open_tcp()
                : Socket::open_tcp6();

            _connect_remote(std::move(sock), addresses);
        }
        break;
    case Command::UdpAssociate:
    case Command::Bind:
    default:
        break;
    }

    return true;
}

bool Session::_connect_remote(
    Socket&& sock,
    const std::vector<IPAddress>* addresses
) {
    _remote_socket = new RemoteSocket(std::move(sock), addresses);
    _remote_socket->set_session(*this);
    if (!_remote_socket->process_event(Event::Error))  // start connect attempts
    {
        return false;
    }
    
    Event remote_event(
        _remote_socket->get_socket().get_fd(),
        static_cast<Event::Flags>(Event::Write | Event::Closed),
        _remote_socket
    );

    _poller.register_event(remote_event);

    _set_state(Session::State::ConnectingRemote);
    return true;
}

void Session::_remote_connected()
{
    Event remote_event(
        _remote_socket->get_socket().get_fd(),
        static_cast<Event::Flags>(Event::Read | Event::Closed),
        _remote_socket
    );

    _poller.update_event(remote_event);
    _set_state(Session::State::Connected);
}

} // namespace sockspp
