#include "server.hpp"
#include "session.hpp"
#include "session_socket.hpp"
#include "defs.hpp"

#include <sockspp/core/s5_enums.hpp>
#include <sockspp/core/poller/poller.hpp>
#include <sockspp/core/poller/event.hpp>
#include <sockspp/core/log.hpp>

#include <vector>
#include <algorithm>

#if defined(_WIN32)
    #define SOCKSPP_POLL_TIMEOUT 2000
#else
    #define SOCKSPP_POLL_TIMEOUT -1
#endif // _WIN32

namespace sockspp::server
{

Server::Server(const ServerParams&& params)
    : _params(std::move(params))
{
    _sessions.reserve(128);
}

const std::string& Server::get_listen_ip() const
{
    return _params.listen_ip;
}

uint16_t Server::get_listen_port() const
{
    return _params.listen_port;
}

AuthMethod Server::get_auth_method() const
{
    if (_params.username.empty() && _params.password.empty())
    {
        return AuthMethod::NoAuth;
    }

    return AuthMethod::UserPass;
}

const std::string& Server::get_dns_ip() const
{
    return _params.dns_ip;
}

uint16_t Server::get_dns_port() const
{
    return _params.dns_port;
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
    events.reserve(SOCKSPP_SERVER_INITIAL_POLL_RESULT_SIZE);

    while (this->is_serving())
    {
        events.clear();

        // workaround for Window because it does not trigger EINTR
        // that's why setting timeout to -1 is not preferred there
        int res = poller.poll(events, SOCKSPP_POLL_TIMEOUT);

        if (res == -1)
        {
            if (sockerrno != EINTR)
            {
                LOGE("Poll error: %d", sockerrno);
                this->stop();
            }
            break;
        }

        if (!res)
        {
#if !defined(_WIN32)
            LOGD("Poller timeout");
#endif
            continue;
        }

        for (int i = 0; i < events.size(); i++)
        {
            Event& event = events[i];
            Event::Flags flags = event.get_flags();

            // server 
            if (event.get_ptr() == reinterpret_cast<void*>(this))
            {
                if (flags & Event::Read)
                {
                    Socket client = _accept_client();
                    _create_new_session(poller, std::move(client));
                }
                else
                {
                    LOGE("Server error");
                    this->stop();
                    break;
                }
            }

            // session
            else if (event.get_ptr())
            {
                SessionSocket* session_socket = \
                    reinterpret_cast<SessionSocket*>(event.get_ptr());

                if (!session_socket->process_event(flags))
                {
                    // delete session when socket is closed
                    Session* session = &session_socket->get_session();

                    // invalidate all next events associated
                    // with the deleted session if there are any
                    for (int j = i; j < events.size(); j++)
                    {
                        Event& event2 = events[j];
                        if (event2.get_ptr() && event2.get_ptr() != this)
                        {
                            SessionSocket* session_socket2 = \
                                reinterpret_cast<SessionSocket*>(event2.get_ptr());

                            if (session == &session_socket2->get_session())
                            {
                                events[j] = Event(0, Event::Closed, nullptr);
                            }
                        }
                    }

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

    _delete_all_sessions();

    if (this->is_serving())
    {
        this->stop();
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
    Socket client = _server_socket.accept();
    return client;
}

Session* Server::_create_new_session(Poller& poller, Socket&& sock)
{
    Session* session = new Session(*this, poller, std::move(sock));
    session->initialize();
    _sessions.push_back(session);
    return session;
}

void Server::_delete_session(Session* session)
{
    session->shutdown();
    _sessions.erase(std::find(
        _sessions.begin(),
        _sessions.end(),
        session
    ));
    delete session;
}

void Server::_delete_all_sessions()
{
    for (auto session : _sessions)
    {
        delete session;
    }

    _sessions.clear();
}

} // namespace sockspp::server
