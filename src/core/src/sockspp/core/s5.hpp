#pragma once

#include "s5_enums.hpp"

namespace sockspp
{

class Address
{
public:
    Address(void* data);

    void* get_data() const { return _data; }
    size_t get_size() const;

    AddrType get_type() const;
    void set_type(AddrType type);

    uint8_t* get_address() const;

    uint16_t get_port() const;
    void set_port(uint16_t port);

private:
    void* _data;
}; // class Address

class R_Base
{
public:
    R_Base(void* data);

    void* get_data() const { return _data; }
    size_t get_size() const;

    Address get_address() const;

protected:
    uint8_t _get_cmd_or_rep() const;

private:
    void* _data;
}; // class R_Base

class CommandMessage : public R_Base
{
public:
    CommandMessage(void* data) : R_Base(data) {}

    inline Command get_command() const
    {
        return static_cast<Command>(_get_cmd_or_rep());
    }
}; // class CommandMessage

class ReplyMessage : public R_Base
{
public:
    ReplyMessage(void* data) : R_Base(data) {}

    inline Reply get_reply() const
    {
        return static_cast<Reply>(_get_cmd_or_rep());
    }
}; // class CommandMessage

} // namespace sockspp
