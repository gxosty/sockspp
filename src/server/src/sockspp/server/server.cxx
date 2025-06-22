#include "server.hpp"
#include "session.hpp"
#include "session_socket.hpp"
#include "sockspp/core/s5_enums.hpp"
#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/poller/event.hpp>
#include <sockspp/core/log.hpp>

#include <vector>

#define SOCKSPP_SERVER_LISTEN 128
#if defined(_WIN32)
    #define SOCKSPP_POLL_TIMEOUT 2000
#else
    #define SOCKSPP_POLL_TIMEOUT -1
#endif // _WIN32

namespace sockspp
{

Server::Server(const ServerParams&& params)
    : _params(std::move(params)) {}

AuthMethod Server::get_auth_method() const
{
    if (_params.username.empty() && _params.password.empty())
    {
        return AuthMethod::NoAuth;
    }

    return AuthMethod::UserPass;
}

bool Server::authenticate(
    const std::string& username,
    const std::string& password
) const {
    if (this->get_auth_method() == AuthMethod::NoAuth)
        return true;

    return (_params.username == username)
        && (_params.password == password);
}

void Server::serve()
{
    _server_socket = Socket::open_tcp();
    _server_socket.set_blocking(false);
    _server_socket.bind(_params.listen_ip, _params.listen_port);
    LOGI("SOCKS5 serving on %s:%d", _params.listen_ip.c_str(), (int)_params.listen_port);
    _server_socket.listen(SOCKSPP_SERVER_LISTEN);

    Poller poller;
    int server_sock = _server_socket.get_fd();
    
    {
        Event server_event(
            server_sock,
            Event::Read,
            reinterpret_cast<void*>(this)
        );

        if (!poller.register_event(server_event))
        {
            LOGE("Couldn't register server event");
            this->stop();
            return;
        }
    }

    std::vector<Event> events;
    events.reserve(64);

    while (this->is_serving())
    {
        events.clear();

        // IDK why but wepoll does react to incoming clients when
        // timeout is set to -1. For now doing some workaround
        // until I find a better solution
        int res = poller.poll(events, SOCKSPP_POLL_TIMEOUT);

        if (res == -1)
        {
            LOGE("Poll error: %d", sockerrno);
            this->stop();
            break;
        }

        if (!res)
        {
#if !defined(_WIN32)
            LOGD("Poller timeout");
#endif
            continue;
        }

        for (auto& event : events)
        {
            Event::Flags flags = event.get_flags();

            if (event.get_ptr() == reinterpret_cast<void*>(this))
            {
                if (flags & Event::Read)
                {
                    Socket client = _accept_client();
                    _create_new_session(poller, std::move(client));
                }
                else
                {
                    break;
                }
            }
            else
            {
                SessionSocket* session_socket = \
                    reinterpret_cast<SessionSocket*>(event.get_ptr());

                if ((flags & Event::Closed) || !session_socket->process())
                {
                    Session* session = &session_socket->get_session();
                    _delete_session(session);
                }
            }
        }
    }

    {
        Event server_event(
            server_sock,
            Event::Closed,
            nullptr
        );

        poller.remove_event(server_event);
    }
}

void Server::stop()
{
    _server_socket.shutdown();
    _server_socket.close();
}

bool Server::is_serving() const
{
    return _server_socket.get_fd() != -1;
}

Socket Server::_accept_client()
{
    SocketInfo client_info;
    Socket client = _server_socket.accept(&client_info);
    LOGI("Accepted: %s", client_info.str().c_str());
    return client;
}

Session* Server::_create_new_session(Poller& poller, Socket&& sock)
{
    Session* session = new Session(*this, poller, std::move(sock));
    session->initialize();
    return session;
}

void Server::_delete_session(Session* session)
{
    session->shutdown();
    delete session;
}

} // namespace sockspp
