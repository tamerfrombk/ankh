#pragma once

#include <stddef.h>
#include <stdbool.h>

extern const char *CLEAN_BUILT_IN_COMMANDS[];

typedef struct command_t {
    char *cmd;
    char **args;
    size_t args_len;
} command_t;

void command_destroy(command_t *cmd);

void parse_command(const char *line, command_t *cmd);

bool is_built_in_command(const command_t *cmd);
