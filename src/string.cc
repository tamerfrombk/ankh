#include <cctype>
#include <numeric>

#include <fak/string.h>


std::vector<std::string> split_by_whitespace(const std::string& str)
{
    std::vector<std::string> result;

    std::string token;
    for (const auto c : str) {
        if (std::isspace(c)) {
            if (!token.empty()) {
                result.push_back(token);
            }
            token.clear();
        } else {
            token += c;
        }
    }

    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
}

std::string join(const std::vector<std::string>& strs, const std::string& joiner)
{
    return std::accumulate(strs.cbegin(), strs.cend(), std::string(""),
    [&](std::string& accum, const std::string& s) {
        accum += joiner;
        return accum += s;
    });
}