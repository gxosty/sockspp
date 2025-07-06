#include "utils.hpp"

#include <fstream>
#include <string>

namespace sockspp::server
{

#if defined _WIN32

std::vector<std::string> get_dns_nameservers()
{
    return {};
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
