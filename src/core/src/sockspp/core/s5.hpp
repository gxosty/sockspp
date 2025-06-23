#pragma once

#include "s5_enums.hpp"
#include <cstdint>

namespace sockspp
{

class S5Address
{
public:
    S5Address(void* data);

    void* get_data() const { return _data; }
    size_t get_size() const;

    AddrType get_type() const;
    void set_type(AddrType type);

    uint8_t* get_address() const;
    void set_address(uint8_t* address); // depends on type to be set first

    uint16_t get_port() const;
    void set_port(uint16_t port);

private:
    void* _data;
}; // class Address

class S5R_Base
{
public:
    S5R_Base(void* data);

    void* get_data() const { return _data; }
    size_t get_size() const;

    S5Address get_address() const;

    void set_version(uint8_t version);

protected:
    uint8_t _get_cmd_or_rep() const;
    void _set_cmd_or_rep(uint8_t cmd_or_rep);

private:
    void* _data;
}; // class R_Base

class S5CommandMessage : public S5R_Base
{
public:
    S5CommandMessage(void* data) : S5R_Base(data) {}

    inline Command get_command() const
    {
        return static_cast<Command>(_get_cmd_or_rep());
    }

    inline void set_command(Command command)
    {
        _set_cmd_or_rep(static_cast<uint8_t>(command));
    }
}; // class CommandMessage

class S5ReplyMessage : public S5R_Base
{
public:
    S5ReplyMessage(void* data) : S5R_Base(data) {}

    inline Reply get_reply() const
    {
        return static_cast<Reply>(_get_cmd_or_rep());
    }

    inline void set_reply(Reply reply)
    {
        _set_cmd_or_rep(static_cast<uint8_t>(reply));
    }
}; // class CommandMessage

} // namespace sockspp
