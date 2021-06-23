#pragma once

#include <stdexcept>

#include <fmt/core.h>
#include <fmt/format.h>

#include <fak/def.h>

namespace fk::lang {

struct ScanException
    : public std::runtime_error
{
    explicit ScanException(const std::string& msg)
        : std::runtime_error(msg) {}
};

struct InterpretationException
    : public std::runtime_error
{
    explicit InterpretationException(const std::string& msg)
        : std::runtime_error(msg) {}
};

struct ParseException
    : public std::runtime_error
{
    explicit ParseException(const std::string& msg)
        : std::runtime_error(msg) {}
};

template <class E, class... Args>
FK_NO_RETURN void panic(const char *fmt, Args&&... args)
{
    const std::string str = fmt::format(fmt, std::forward<Args>(args)...);

    throw E(str);
}

}