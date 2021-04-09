#include <stdlib.h>

#include <clean/command.h>
#include <clean/string.h>
#include <clean/log.h>

command_t parse_command(const std::string& str)
{
    auto words = split_by_whitespace(str);

    command_t cmd { words[0], words };

    debug("command parsed: %s\n", cmd.cmd.c_str());
    for (size_t i = 1; i < cmd.args.size(); ++i) {
        debug("arg: %s\n", cmd.args[i].c_str());
    }

    return cmd;
}
