#include "utils.hpp"

#include <string>

#if defined _WIN32
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
#endif // _WIN32

#if defined __linux__
    #include <fstream>
#endif // __linux__

namespace sockspp::server
{

#if defined _WIN32

std::vector<std::string> get_dns_nameservers()
{
    PIP_ADAPTER_ADDRESSES pAdapters = NULL;
    ULONG outBufLen = 0;
    ULONG flags = GAA_FLAG_SKIP_MULTICAST;
    std::vector<std::string> nameservers;

    // Get the size of the buffer needed
    GetAdaptersAddresses(AF_INET, flags, NULL, NULL, &outBufLen);

    pAdapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(outBufLen));
    if (pAdapters == NULL) {
        free(pAdapters);
    }

    // Get the adapter addresses
    if (GetAdaptersAddresses(AF_INET, flags, NULL, pAdapters, &outBufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_ADDRESSES pAdapter = pAdapters;
        while (pAdapter) {
            // Check if the interface is up and has a valid IP address
            if (pAdapter->OperStatus != IfOperStatusUp
                || pAdapter->FirstUnicastAddress == NULL) {
                pAdapter = pAdapter->Next;
                continue;
            }

            // Check if the interface has the "Primary Interface" flag
            DWORD flags = pAdapter->IfType;
            bool is_primary = (flags & IF_TYPE_ETHERNET_CSMACD) != 0 && (flags & IF_TYPE_IEEE80211) != 0;

            if (is_primary)
            {
                PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsServerAddress = \
                    pAdapter->FirstDnsServerAddress;

                while (pDnsServerAddress)
                {
                    sockaddr* addr = reinterpret_cast<sockaddr*>(
                        pDnsServerAddress->Address.lpSockaddr
                    );

                    std::string address_str;
                    
                    if (addr->sa_family == AF_INET)
                    {
                        char ipstr[16];
                        memset(ipstr, 0, 16);
                        inet_ntop(
                            AF_INET,
                            &reinterpret_cast<sockaddr_in*>(addr)->sin_addr,
                            ipstr,
                            16
                        );
                        address_str += std::string(ipstr);
                    }
                    else if (addr->sa_family == AF_INET6)
                    {
                        char ipstr[40];
                        memset(ipstr, 0, 40);
                        inet_ntop(
                            AF_INET6,
                            &reinterpret_cast<sockaddr_in6*>(addr)->sin6_addr,
                            ipstr,
                            40
                        );
                        address_str += std::string(ipstr);
                    }

                    nameservers.push_back(address_str);

                    pDnsServerAddress = pDnsServerAddress->Next;
                }

                pAdapter = nullptr;
            }
            else
            {
                pAdapter = pAdapter->Next;
            }
        }
    }

    free(pAdapters);
    return nameservers;
}

#elif defined __linux__

std::vector<std::string> get_dns_nameservers()
{
    std::ifstream resolv_file("/etc/resolv.conf");

    if (!resolv_file.is_open())
        return {};

    std::string line;
    std::vector<std::string> nameservers;

    while (std::getline(resolv_file, line))
    {
        if (line.starts_with("nameserver "))
        {
            nameservers.push_back(line.substr(11));
        }
    }

    return nameservers;
}

#else

std::vector<std::string> get_dns_nameservers()
{
    return {};
}

#endif

} // namespace sockspp::server
