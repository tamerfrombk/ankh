#pragma once

#include <stdexcept>

namespace fk::lang {

struct ParseException
    : public std::runtime_error
{
    explicit ParseException(const std::string& msg)
        : std::runtime_error(msg) {}
    
    explicit ParseException(const char *msg)
        : std::runtime_error(msg) {}
};

}