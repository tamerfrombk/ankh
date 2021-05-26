#pragma once

#include <stdexcept>

namespace fk::lang {

struct InterpretationException
    : public std::runtime_error
{
    explicit InterpretationException(const std::string& msg)
        : std::runtime_error(msg) {}
    
    explicit InterpretationException(const char *msg)
        : std::runtime_error(msg) {}
};

}