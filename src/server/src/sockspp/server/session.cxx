#include "session.hpp"
#include "server.hpp"
#include "defs.hpp"

#include <sockspp/core/s5.hpp>
#include <sockspp/core/errno.hpp>
#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/poller/event.hpp>
#include <sockspp/core/log.hpp>

#include <cerrno>
#include <exception>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif // _WIN32

namespace sockspp::server
{

Session::Session(
    const Server& server,
    Poller& poller,
    Socket&& sock
)   : _server(server)
    , _poller(poller)
    , _client_socket(new ClientSocket(std::move(sock)))
    , _remote_socket(nullptr)
    , _udp_socket(nullptr)
    , _client_buffer(SOCKSPP_SESSION_SOCKET_BUFFER_SIZE)
    , _remote_buffer(SOCKSPP_SESSION_SOCKET_BUFFER_SIZE) {}

Session::~Session()
{
    // delete sockets associated with this session

    delete _client_socket;

    if (_remote_socket)
        delete _remote_socket;

    if (_udp_socket)
        delete _udp_socket;

    for (auto dns_socket : _dns_sockets)
    {
        delete dns_socket;
    }
}

void Session::initialize()
{
    _client_socket->set_session(*this);

    _poller.set_event(
        _client_socket->get_socket().get_fd(),
        _client_socket,
        static_cast<Event::Flags>(Event::Read | Event::Closed)
    );

    _peer_info = _client_socket->get_socket().get_peer_address();

    _state = Session::State::Accepted;
}

void Session::shutdown()
{
    LOGI("Shutdown cli:%s", _peer_info.str().c_str());

    // shutdown and unregister all sockets associated with this session

    _poller.remove_event(_client_socket->get_socket().get_fd());
    _client_socket->get_socket().shutdown();
    _client_socket->get_socket().close();

    if (_remote_socket)
    {
        _poller.remove_event(_remote_socket->get_socket().get_fd());
        _remote_socket->get_socket().shutdown();
        _remote_socket->get_socket().close();
    }

    if (_udp_socket)
    {
        _poller.remove_event(_udp_socket->get_socket().get_fd());
        _udp_socket->get_socket().shutdown();
        _udp_socket->get_socket().close();
    }

    for (auto dns_socket : _dns_sockets)
    {
        _poller.remove_event(dns_socket->get_socket().get_fd());
        dns_socket->get_socket().shutdown();
        dns_socket->get_socket().close();
    }
}

bool Session::process_client_event(Event::Flags event_flags)
{
    if (event_flags & (Event::Closed | Event::Error))
    {
        return false;
    }

    if (event_flags & Event::Write)
    {
        return _session_socket_send(_client_socket, nullptr, _client_buffer);
    }

    uint8_t _buffer[SOCKSPP_SESSION_SOCKET_BUFFER_SIZE];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        SOCKSPP_SESSION_SOCKET_BUFFER_SIZE
    );

    int status = _client_socket->recv(buffer);

    if (status == 0)
    {
        return false;
    }
    else if (status == -1)
    {
        LOGE("Client receive error (errno: %d, session state: %d)", sockerrno, (int)_state);
        return false;
    }

    return _process_client(buffer, nullptr, 0);
}

bool Session::process_remote_event(Event::Flags event_flags)
{
    if (event_flags & Event::Closed)
    {
        return false;
    }

    if (event_flags & Event::Error)
    {
        if (!_remote_socket->is_connected())
        {
            return _remote_socket->try_connect_next();
        }

        return false;
    }

    if (event_flags & Event::Write)
    {
        if (!_remote_socket->is_connected())
            return _remote_socket->could_connect();

        return _session_socket_send(_remote_socket, nullptr, _remote_buffer);
    }

    uint8_t _buffer[SOCKSPP_SESSION_SOCKET_BUFFER_SIZE];
    MemoryBuffer buffer(
        reinterpret_cast<void*>(_buffer),
        0,
        SOCKSPP_SESSION_SOCKET_BUFFER_SIZE
    );

    int status = -1;
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    if (_state == Session::State::Connected)
    {
        status = _remote_socket->recv(buffer);
    }
    else if (_state == Session::State::Associated)
    {
        status = _remote_socket->recv_from(
            buffer,
            &addr,
            reinterpret_cast<int*>(&addr_len)
        );
    }

    if (status == 0)
    {
        return false;
    }
    else if (status == -1)
    {
        LOGE("Remote receive error (errno: %d, session state: %d)", sockerrno, (int)_state);
        return false;
    }

    return _process_remote(buffer, &addr, addr_len);
}

bool Session::process_udp_event(Event::Flags event_flags)
{
    if (event_flags & (Event::Closed | Event::Error))
    {
        return false;
    }

    char _buffer[SOCKSPP_SESSION_SOCKET_BUFFER_SIZE];
    MemoryBuffer buffer(
        _buffer,
        0,
        SOCKSPP_SESSION_SOCKET_BUFFER_SIZE
    );

    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    int status = _udp_socket->recv_from(
        buffer,
        &addr,
        reinterpret_cast<int*>(&addr_len)
    );

    if (status == 0)
    {
        // invalid source ip, so we drop the packet
        return true;
    }
    else if (status == -1)
    {
        return false;
    }

    return _process_client(buffer, &addr, addr_len);
}

bool Session::process_dns_event(Event::Flags event_flags, DnsSocket* dns_socket)
{
    if (event_flags & (Event::Closed | Event::Error))
    {
        return false;
    }

    std::vector<IPAddress>* addresses = new std::vector<IPAddress>();
    int status = dns_socket->get_response(addresses);

    _dns_sockets.erase(std::find(
        _dns_sockets.begin(),
        _dns_sockets.end(),
        dns_socket
    ));

    _poller.remove_event(dns_socket->get_socket().get_fd());

    delete dns_socket;

    if (status == 0)
    {
        LOGE("DNS Response size: 0");
        return 0;
    }
    else if (status == -1)
    {
        LOGE("DNS Response receive error (errno: %d)", sockerrno);
        return -1;
    }

    return _do_command(addresses);
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

bool Session::_process_client(MemoryBuffer& buffer, void* addr, int addr_len)
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
    case Session::State::ResolvingDomainName:
        LOGE("Client event occured when resolving domain name");
        return false;
    case Session::State::Connected:
        LOGD(
            "TCP | %s -> %s | %zu",
            _peer_info.str().c_str(),
            _remote_socket->get_remote_info().str().c_str(),
            buffer.get_size()
        );

        return _session_socket_send(_remote_socket, &buffer, _remote_buffer);
    case Session::State::Associated:
        return _remote_socket->send_to(buffer, addr, addr_len);
    default:
        break;
    }

    LOGD("Undefined session state (Client)");
    return false;
}

bool Session::_process_remote(MemoryBuffer& buffer, void* addr, int addr_len)
{
    switch (_state)
    {
    case Session::State::Connected:
        LOGD(
            "TCP | %s <- %s | %zu",
            _peer_info.str().c_str(),
            _remote_socket->get_remote_info().str().c_str(),
            buffer.get_size()
        );
        return _session_socket_send(_client_socket, &buffer, _client_buffer);
    case Session::State::Associated:
        return _udp_socket->send_to(buffer, addr, addr_len);
    default:
        break;
    }

    LOGD("Undefined session state (Remote)");
    return false;
}

bool Session::_session_socket_send(
    SessionSocket* session_socket,
    MemoryBuffer* buffer,
    MemoryBuffer& scheduled
) {
    bool is_scheduled = scheduled.get_size() > 0;
    MemoryBuffer& send_buffer = is_scheduled ? scheduled : *buffer;
    int res = session_socket->send(send_buffer);

    if (res != send_buffer.get_size())
    {
        if (
            (res == -1)
            && (sockerrno != EWOULDBLOCK)
            && (sockerrno != EAGAIN)
        ) {
            // Error occured
            return false;
        }

        // Schedule buffer for next WRITE event
        if (!is_scheduled || (res != -1))
        {
            size_t sent = res == -1 ? 0 : res;
            size_t copy_size = send_buffer.get_size() - sent;
            scheduled.copy_from(buffer->as<uint8_t*>() + sent, copy_size);
        }

        // Listen for WRITE event
        if (!is_scheduled)
        {
            _poller.set_event(
                session_socket->get_socket().get_fd(),
                session_socket,
                static_cast<Event::Flags>(Event::Write | Event::Closed),
                true
            );
        }
    }
    else if (is_scheduled)
    {
        // Sent scheduled buffer
        scheduled.set_size(0);

        // Listen for READ event
        _poller.set_event(
            session_socket->get_socket().get_fd(),
            session_socket,
            static_cast<Event::Flags>(Event::Read | Event::Closed),
            true
        );
    }

    return true;

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
    case Command::UdpAssociate:
        break;
    case Command::Bind:
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
        *is_domain_name = _resolve_domain_name(buffer);
        if (!*is_domain_name)
        {
            LOGD("Attempted to resolve domain, but dns is `None`");
            _client_socket->send_reply(
                Reply::AddrTypeNotSupported,
                type,
                address.get_address(),
                address.get_port()
            );
        }
        return nullptr;
    default:
        LOGE("Unsupported address type: %d", static_cast<int>(type));
        _client_socket->send_reply(
            Reply::AddrTypeNotSupported,
            type,
            address.get_address(),
            address.get_port()
        );
        break;
    }

    return nullptr;
}

bool Session::_resolve_domain_name(MemoryBuffer& buffer)
{
    if (_dns_sockets.size() >= SOCKSPP_SESSION_MAX_DNS_SOCKETS)
    {
        return false;
    }

    if (_server.get_dns_ip().empty())
    {
        return false;
    }

    IPAddress dns_address(
        _server.get_dns_ip(),
        _server.get_dns_port()
    );

    Socket sock = dns_address.get_version() == IPAddress::Version::IPv4
        ? Socket::open_udp()
        : Socket::open_udp6();

    S5CommandMessage message(buffer.as<uint8_t*>());
    S5Address address = message.get_address();

    uint8_t* domain_name_data = address.get_address();
    _domain_name = std::string((char*)domain_name_data+1, *domain_name_data);
    uint16_t port = address.get_port();

    DnsSocket* dns_socket = new DnsSocket(
        std::move(sock),
        _domain_name,
        port
    );

    dns_socket->set_session(*this);

    _dns_sockets.push_back(dns_socket);

    _poller.set_event(
        dns_socket->get_socket().get_fd(),
        dns_socket,
        static_cast<Event::Flags>(Event::Read | Event::Closed)
    );

    _set_state(Session::State::ResolvingDomainName);
    int status = dns_socket->query(dns_address);

    if (status == 0)
    {
        LOGE("DNS Query error");
        return false;
    }
    else if (status == -1)
    {
        LOGE("DNS Query error (errno: %d)", sockerrno);
        return false;
    }

    return status > 0;
}

bool Session::_do_command(
    const std::vector<IPAddress>* addresses
) {
    switch (_command)
    {
    case Command::Connect:
        {
            Socket sock =
                addresses->at(0).get_version() == IPAddress::Version::IPv4
                ? Socket::open_tcp()
                : Socket::open_tcp6();

            _connect_remote(std::move(sock), addresses);
        }
        break;
    case Command::UdpAssociate:
        {
            bool is_ipv4 = addresses->at(0).get_version() == IPAddress::Version::IPv4;

            Socket cl_sock = is_ipv4
                ? Socket::open_udp()
                : Socket::open_udp6();

            Socket rm_sock = is_ipv4
                ? Socket::open_udp()
                : Socket::open_udp6();

            return _associate(std::move(cl_sock), std::move(rm_sock));
        }
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
    _set_state(Session::State::ConnectingRemote);
    if (!_remote_socket->process_event(Event::Error))  // start connect attempts
    {
        return false;
    }
    
    _poller.set_event(
        _remote_socket->get_socket().get_fd(),
        _remote_socket,
        static_cast<Event::Flags>(Event::Write | Event::Closed)
    );

    return true;
}

void Session::_remote_connected()
{
    _poller.set_event(
        _remote_socket->get_socket().get_fd(),
        _remote_socket,
        static_cast<Event::Flags>(Event::Read | Event::Closed),
        true
    );

#if !SOCKSPP_DISABLE_LOGS
    SocketInfo remote_info = _remote_socket->get_remote_info();

    LOGI(
        "TCP CONNECT | cli:%s <-> rem:%s%s",
        _peer_info.str().c_str(),
        remote_info.str().c_str(),
        _domain_name.empty()
            ? ""
            : (" (" + _domain_name + ")").c_str()
    );
#endif

    _set_state(Session::State::Connected);
}

bool Session::_associate(Socket&& cl_sock, Socket&& rm_sock)
{
    sockaddr_storage bind_addr;
    socklen_t bind_addr_len = sizeof(bind_addr);
    SocketInfo client_bound_info = _client_socket->get_socket() .get_bound_address();
    client_bound_info.to(
        &bind_addr,
        reinterpret_cast<int*>(&bind_addr_len)
    );

    if (bind_addr.ss_family == AF_INET)
        reinterpret_cast<sockaddr_in*>(&bind_addr)->sin_port = 0;
    else
        reinterpret_cast<sockaddr_in6*>(&bind_addr)->sin6_port = 0;

    if (cl_sock.bind(&bind_addr, static_cast<int>(bind_addr_len)) == -1)
    {
        LOGE("cl_sock.bind() == -1: %d", sockerrno);
        return false;
    }

    SocketInfo bound_info;

    try {
        bound_info = cl_sock.get_bound_address();
    } catch (const std::exception& ex) {
        LOGE("cl_sock.get_bound_address()");
        return false;
    }
    
    if (!_client_socket->send_reply(
        Reply::Success,
        bound_info.ip_version == SocketInfo::IPv4
            ? AddrType::IPv4
            : AddrType::IPv6,
        bound_info.ip,
        ntohs(bound_info.port)
    )) {
        LOGD("failed sending association reply");
        return false;
    }

    _udp_socket = new UDPSocket(std::move(cl_sock), _peer_info);
    _udp_socket->set_session(*this);

    _poller.set_event(
        _udp_socket->get_socket().get_fd(),
        _udp_socket,
        static_cast<Event::Flags>(Event::Read | Event::Closed)
    );

    _remote_socket = new RemoteSocket(std::move(rm_sock), nullptr);
    _remote_socket->set_session(*this);

    _poller.set_event(
        _remote_socket->get_socket().get_fd(),
        _remote_socket,
        static_cast<Event::Flags>(Event::Read | Event::Closed)
    );

    LOGI(
        "UDP ASSOCIATE | cli:%s <-> bnd:%s",
        _peer_info.str().c_str(),
        bound_info.str().c_str()
    );

    _set_state(Session::State::Associated);
    return true;
}

} // namespace sockspp::server
