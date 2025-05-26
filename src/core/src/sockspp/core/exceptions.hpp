#pragma once

#include <exception>
#include <string>
#include <cstdlib>

namespace sockspp
{

class Exception : public std::exception
{
public:
    explicit Exception(std::string message, std::string where = "", int code = 0)
    : _message(std::move(message)), _where(std::move(where)), _code(code) {}

    [[nodiscard]] const char* what() const noexcept override
    {
        return _message.c_str();
    }

    [[nodiscard]] virtual std::string full_message() const
    {
        std::string message = _message;

        if (!_where.empty())
        {
            message += "\nWhere: " + _where;
        }

        if (_code)
        {
            message += "\nCode: " + std::to_string(_code);
        }

        return message;
    }

    [[nodiscard]] int code() const noexcept
    {
        return _code;
    }

private:
    std::string _message;
    std::string _where;
    int _code;
}; // class CoreException

class MemoryAllocationError : public Exception
{
public:
    explicit MemoryAllocationError(size_t allocation_size)
    : Exception("Couldn't allocate " + std::to_string(allocation_size) + " bytes of memory") {}
}; // class MemoryAllocationError

} // namespace sockspp
