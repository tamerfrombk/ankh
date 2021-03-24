#pragma once

// Forward declarations
struct command_t;

typedef struct variable_t {
    char *name, *value;
    struct variable_t *next;
} variable_t;

typedef struct shell_t {
    variable_t *vars;
} shell_t;

void shell_init(shell_t *sh);
void shell_teardown(shell_t *sh);

char *shell_find_variable_value(shell_t *sh, const char *var);

int shell_loop(int argc, char **argv);

int execute(shell_t *sh, const struct command_t *cmd);

char *substitute_variables(shell_t *sh, const char *str);
