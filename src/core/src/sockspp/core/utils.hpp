#pragma once

#include <string>

namespace sockspp
{

bool is_ipv4_address(const std::string& address);
bool is_ipv6_address(const std::string& address);
bool is_ip_address(const std::string& address);

} // namespace sockspp
