#pragma once

#include <stdexcept>

namespace fk::lang {

struct interpretation_exception
    : public std::runtime_error
{
    explicit interpretation_exception(const std::string& msg)
        : std::runtime_error(msg) {}
    
    explicit interpretation_exception(const char *msg)
        : std::runtime_error(msg) {}
};

}