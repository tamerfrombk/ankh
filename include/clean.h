#pragma once

// Forward declarations
struct command_t;

int shell_loop(int argc, char **argv);

int execute(const struct command_t *cmd);