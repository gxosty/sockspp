#pragma once

#include "session_socket.hpp"
#include <sockspp/core/memory_buffer.hpp>
#include <sockspp/core/buffer.hpp>
#include <sockspp/core/ip_address.hpp>

#include <string>
#include <vector>

namespace sockspp::server
{

class Session;

class DnsSocket : public SessionSocket
{
public:
    DnsSocket(
        Socket&& sock,
        const std::string& domain_name,
        uint16_t port,
        sockspp::MemoryBuffer* buffer = nullptr
    );

    const sockspp::Buffer& get_buffer() const;

    int query(const IPAddress& dns_address);
    int get_response(std::vector<IPAddress>* addresses);

    bool process_event(Event::Flags event_flags) override;

private:
    sockspp::Buffer _buffer;
    std::string _domain_name;
    uint16_t _port;


}; // class DnsSocket

} // namespace sockspp::server
