#pragma once

#include <vector>
#include <string>

std::vector<std::string> split_by_whitespace(const std::string& str);

std::string join(const std::vector<std::string>& strs, const std::string& joiner);
