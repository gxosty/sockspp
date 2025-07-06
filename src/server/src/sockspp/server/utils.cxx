#include "utils.hpp"

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
    return {};
}

#else

std::vector<std::string> get_dns_nameservers()
{
    return {};
}

#endif

} // namespace sockspp::server
