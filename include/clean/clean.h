#pragma once

// Forward declarations
struct command_t;
struct lexer_t;

typedef struct variable_t {
    char *name, *value;
    struct variable_t *next;
} variable_t;
typedef struct shell_t {
    variable_t *vars;
} shell_t;

typedef enum expr_result_type {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NIL,
    RT_ERROR
} expr_result_type;

typedef struct expr_result_t {
    union {
        char *str;
        char err[31 + 1];
        int exit_code;
    };
    expr_result_type type;
} expr_result_t;

void shell_init(shell_t *sh);
void shell_teardown(shell_t *sh);

char *shell_find_variable_value(shell_t *sh, const char *var);

int shell_loop(int argc, char **argv);

int shell_consume_statement(shell_t *sh, const char *stmt);
void shell_consume_export_statement(shell_t *sh, struct lexer_t *lexer);

expr_result_t shell_evaluate_expression(shell_t *sh, const char *line);

int execute(shell_t *sh, const struct command_t *cmd);

char *substitute_variables(shell_t *sh, const char *str);
