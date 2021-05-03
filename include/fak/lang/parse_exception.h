#pragma once

#include <stdexcept>

namespace fk::lang {

struct parse_exception
    : public std::runtime_error
{
    explicit parse_exception(const std::string& msg)
        : std::runtime_error(msg) {}
    
    explicit parse_exception(const char *msg)
        : std::runtime_error(msg) {}
};

}