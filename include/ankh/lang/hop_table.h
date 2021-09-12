#pragma once

#include <cstddef>
#include <unordered_map>

namespace ankh::lang {

using HopTable = std::unordered_map<const void*, size_t>;

}