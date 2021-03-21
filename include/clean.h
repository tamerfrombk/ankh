#pragma once

#include <stdbool.h>

int shell_loop(int argc, char **argv);

bool execute( char *const cmd[]);