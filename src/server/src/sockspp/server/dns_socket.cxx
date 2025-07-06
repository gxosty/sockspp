#include "dns_socket.hpp"
#include "session.hpp"

#include <sockspp/core/log.hpp>
#include <sockspp/core/errno.hpp>

#include <dnslib/dns.h>
#include <dnslib/message.h>
#include <dnslib/qs.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

namespace sockspp::server
{

DnsSocket::DnsSocket(
    Socket&& sock,
    const std::string& domain_name,
    uint16_t port,
    sockspp::MemoryBuffer* buffer)
    : SessionSocket(std::move(sock))
    , _domain_name(domain_name)
    , _port(port)
{
    if (buffer)
    {
        _buffer = sockspp::Buffer(buffer->get_size());
        _buffer.copy_from(*buffer);
    }
}

const sockspp::Buffer& DnsSocket::get_buffer() const
{
    return _buffer;
}

int DnsSocket::query(const IPAddress& dns_address)
{
    dns::Message message;

    message.setQr(0);
    message.setOpCode(0);
    auto qs = new dns::QuerySection(_domain_name);
    qs->setType(dns::RDATA_A);
    message.addQuery(qs);

    LOGD("dns message: %s", message.asString().c_str());

    char _buffer[dns::MAX_MSG_LEN];
    uint32_t buffer_size = 0;
    message.encode(_buffer, dns::MAX_MSG_LEN, buffer_size);
    MemoryBuffer buffer(_buffer, buffer_size, dns::MAX_MSG_LEN);

    sockaddr_storage send_addr;
    socklen_t send_addr_len;

    if (dns_address.get_version() == IPAddress::Version::IPv4)
    {
        send_addr.ss_family = AF_INET;
        sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&send_addr);
        s->sin_addr.s_addr =
            *reinterpret_cast<uint32_t*>(dns_address.get_address());
        s->sin_port = dns_address.get_netport();
        send_addr_len = sizeof(sockaddr_in);
    }
    else
    {
        send_addr.ss_family = AF_INET6;
        sockaddr_in6* s =
            reinterpret_cast<sockaddr_in6*>(&send_addr);
        s->sin6_port = dns_address.get_netport();
        memcpy(&s->sin6_addr, dns_address.get_address(), sizeof(s->sin6_addr));
        send_addr_len = sizeof(sockaddr_in6);
    }

    LOGD("DNS Query: %s", _domain_name.c_str());

    return SessionSocket::send_to(
        buffer,
        reinterpret_cast<sockaddr*>(&send_addr),
        send_addr_len
    );
}

int DnsSocket::get_response(std::vector<IPAddress>* addresses)
{
    char _buffer[dns::MAX_MSG_LEN];
    MemoryBuffer buffer(_buffer, 0, dns::MAX_MSG_LEN);

    int size = SessionSocket::recv_from(buffer, nullptr, nullptr);

    if (size <= 0)
    {
        LOGE("size");
        return size;
    }

    dns::Message message;
    message.decode(_buffer, buffer.get_size());

    LOGD("dns response: %s", message.asString().c_str());

    for (auto answer : message.getAnswers())
    {
        dns::eClass cls = answer->getClass();

        if (cls == dns::CLASS_IN)
        {
            dns::eRDataType type = answer->getType();

            if (type == dns::RDATA_A)
            {
                dns::RDataA* rdata = \
                    reinterpret_cast<dns::RDataA*>(answer->getRData());

                addresses->emplace_back(
                    IPAddress::Version::IPv4,
                    rdata->getAddress(),
                    _port
                );
            }

            if (type == dns::RDATA_AAAA)
            {
                dns::RDataAAAA* rdata = \
                    reinterpret_cast<dns::RDataAAAA*>(answer->getRData());

                addresses->emplace_back(
                    IPAddress::Version::IPv6,
                    rdata->getAddress(),
                    _port
                );
            }
        }
    }

    return size;
}

bool DnsSocket::process_event(Event::Flags event_flags)
{
    return get_session().process_dns_event(event_flags, this);
}

} // namespace sockspp::server
