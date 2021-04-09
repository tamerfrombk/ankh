#pragma once

#include <stddef.h>

#include <string>
#include <vector>

struct command_t {
    std::string cmd;
    std::vector<std::string> args;
};

command_t parse_command(const std::string& str);