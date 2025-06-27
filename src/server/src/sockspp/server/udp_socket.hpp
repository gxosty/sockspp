#pragma once

#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/server/session_socket.hpp>

#include <unordered_map>
#include <cstdint>

namespace sockspp
{

class UDPSocket : public SessionSocket
{
public:
    UDPSocket(Socket&& sock, SocketInfo client_info);

    bool process_event(Event::Flags event_flags) override;

    int recv_from(MemoryBuffer& buffer, void* addr, int* addr_len) override;
    int send_to(MemoryBuffer& buffer, void* addr, int addr_len) override;

private:
    std::unordered_map<uint16_t, uint16_t> _port_maps;
    SocketInfo _client_info;

}; // class UDPSocket

}; // class UDPSocket
