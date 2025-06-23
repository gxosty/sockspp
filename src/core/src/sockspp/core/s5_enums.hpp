#pragma once

#include <cstdint>

namespace sockspp
{
    enum class AuthMethod : uint8_t
    {
        NoAuth = 0x00,
        UserPass = 0x02,
        Invalid = 0xFF,
    };

    enum class AddrType : uint8_t
    {
        IPv4 = 0x01,
        DomainName = 0x03,
        IPv6 = 0x04,
        Invalid = 0xFF,
    };

    enum class Command : uint8_t
    {
        Connect = 0x01,
        Bind = 0x02,
        UdpAssociate = 0x03,
        Invalid = 0xFF,
    };

    enum class Reply : uint8_t
    {
        Success = 0x00,
        GeneralFailure = 0x01,
        NotAllowed = 0x02,
        Unreachable = 0x03,
        HostUnreachable = 0x04,
        ConnectionRefused = 0x05,
        TTLExpired = 0x06,
        CommandNotSupported = 0x07,
        AddrTypeNotSupported = 0x08,
        Invalid = 0xFF,
    };
}
