#pragma once

#include <stdbool.h>

// Forward declarations
struct command_t;

int shell_loop(int argc, char **argv);

bool execute(const struct command_t *cmd);