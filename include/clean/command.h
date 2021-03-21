#pragma once

#include <stddef.h>

typedef struct command_t {
    char *cmd;
    char **args;
    size_t args_len;
} command_t;

void command_destroy(command_t *cmd);

void parse_command(const char *line, command_t *cmd);