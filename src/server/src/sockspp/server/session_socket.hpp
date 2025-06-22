#pragma once

#include <sockspp/core/socket.hpp>
#include <sockspp/core/poller/poller.hpp>

namespace sockspp
{

class Session;

class SessionSocket
{
public:
    friend class Session;

public:
    SessionSocket() = default;
    SessionSocket(const SessionSocket& other) = delete;

    SessionSocket(SessionSocket&& other)
    {
        _session = other._session;        
        _sock = std::move(
            other._sock
        );
    }

    SessionSocket(const Socket& sock) = delete;
    SessionSocket(Socket&& sock) : _sock(std::move(sock)) {}

    virtual ~SessionSocket()
    {
        _session = nullptr;
        _sock.close();
    }

    virtual bool process() = 0;

    inline Session& get_session() const
    {
        // be careful, I am lazy to improve this part
        return const_cast<Session&>(*_session);
    }

protected:
    inline void set_session(const Session& session)
    {
        _session = &session;
    }

    Socket& get_socket()
    {
        return _sock;
    }

private:
    const Session* _session = nullptr;
    Socket _sock;

}; // class SessionSocket

}; // namespace sockspp
