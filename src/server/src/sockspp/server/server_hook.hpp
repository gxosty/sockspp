#pragma once

#include <sockspp/core/socket.hpp>
#include <sockspp/core/memory_buffer.hpp>

#include "client_socket.hpp"
#include "remote_socket.hpp"
#include "udp_socket.hpp"

namespace sockspp::server
{

class Server;
class Session;

class ServerHook
{
public:
    // event hooks
    virtual void on_server_started(const Server& server) {}
    virtual void on_server_stopped(const Server& server) {}
    virtual void on_server_accepted_client(const Server& server, const ClientSocket& client) {}
    virtual void on_client_disconnected(const Server& server, const ClientSocket& client) {}
    virtual void on_remote_socket_created(const Server& server, const Socket& remote_socket) {}
    virtual void on_remote_connected(const Server& server, const RemoteSocket& remote) {}
    virtual void on_remote_disconnected(const Server& server, const RemoteSocket& remote) {}

    // behavior hooks
    virtual ClientSocket* create_client_socket(Socket&& sock)
    {
        return new ClientSocket(std::move(sock));
    }

    virtual RemoteSocket* create_remote_socket(Socket&& sock, const std::vector<IPAddress>* addresses)
    {
        return new RemoteSocket(std::move(sock), addresses);
    }

    virtual UDPSocket* create_udp_socket(Socket&& sock, const SocketInfo& client_info)
    {
        return new UDPSocket(std::move(sock), client_info);
    }
    
    virtual sockspp::Socket client_accept(sockspp::Socket& server_socket)
    {
        return server_socket.accept();
    }

    virtual int client_send(ClientSocket& client_socket, MemoryBuffer& buffer)
    {
        return client_socket.send(buffer);
    }

    virtual int client_recv(ClientSocket& client_socket, MemoryBuffer& buffer)
    {
        return client_socket.recv(buffer);
    }

    virtual int remote_send(RemoteSocket& remote_socket, MemoryBuffer& buffer)
    {
        return remote_socket.send(buffer);
    }

    virtual int remote_recv(RemoteSocket& remote_socket, MemoryBuffer& buffer)
    {
        return remote_socket.recv(buffer);
    }

    virtual int udp_send_to(UDPSocket& udp_socket, MemoryBuffer& buffer, void* addr, int addr_len)
    {
        return udp_socket.send_to(buffer, addr, addr_len);
    }

    virtual int udp_recv_from(UDPSocket& udp_socket, MemoryBuffer& buffer, void* addr, int* addr_len)
    {
        return udp_socket.recv_from(buffer, addr, addr_len);
    }
}; // class ServerHook

} // namespace sockspp::server
