#pragma once

#include <cstdlib>

#include <string>

namespace ankh::sys {

inline bool setenv(const std::string &name, const std::string &value) noexcept {
    return ::setenv(name.c_str(), value.c_str(), true) == 0;
}

} // namespace ankh::sys
