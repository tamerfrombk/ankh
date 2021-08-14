#pragma once

#include <string_view>

namespace ankh::pkg {

std::string_view parse_file_name_from_full_path(const std::string_view& path) noexcept;

}