#pragma once

#include <sockspp/core/socket.hpp>
#include <sockspp/core/poller/poller.hpp>

namespace sockspp::server
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

    virtual bool process_event(Event::Flags event_flags) = 0;

    // so that you can add some vpn functionality
    virtual int recv(MemoryBuffer& buffer)
    {
        Socket& sock = this->get_socket();
        
        int size = sock.recv(buffer.as<char*>(), buffer.get_capacity(), 0);
        buffer.set_size(size > 0 ? size : 0);

        return size;
    }

    virtual int recv_from(
        MemoryBuffer& buffer,
        void* sock_addr,
        int* sock_addr_len
    ) {
        Socket& sock = this->get_socket();

        int size = sock.recv_from(
            buffer.as<char*>(),
            buffer.get_capacity(),
            sock_addr,
            sock_addr_len,
            0
        );

        buffer.set_size(size > 0 ? size : 0);

        return size;
    }

    virtual int send(MemoryBuffer& buffer)
    {
        Socket& sock = this->get_socket();
        return sock.send(buffer.as<char*>(), buffer.get_size(), 0);
    }

    virtual int send_to(
        MemoryBuffer& buffer,
        void* sock_addr,
        int sock_addr_len
    ) {
        Socket& sock = this->get_socket();

        return sock.send_to(
            buffer.as<const char*>(),
            buffer.get_size(),
            sock_addr,
            sock_addr_len,
            0
        );
    }

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

}; // namespace sockspp::server
