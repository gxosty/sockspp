#include "utils.hpp"

#include <regex>

namespace sockspp
{

bool is_ipv4_address(const std::string& address)
{
    static const std::regex pattern(
        "\\b((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]"
        "|[01]?[0-9][0-9]?)\\b"
    );

    return std::regex_match(address, pattern);
}

bool is_ipv6_address(const std::string& address)
{
    static const std::regex pattern(
        "\\b([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}\\b|\\b((:[0-9a-fA-F]{1,4}){"
        "1,7}|[0-9a-fA-F]{1,4}(:[0-9a-fA-F]{1,4}){0,6})::[0-9a-fA-F]{1,4}(:[0-"
        "9a-fA-F]{1,4}){0,6}\\b"
    );

    return std::regex_match(address, pattern);
}

bool is_ip_address(const std::string& address)
{
    return is_ipv4_address(address)
        || is_ipv6_address(address);
}

} // namespace sockspp
