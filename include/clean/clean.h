#pragma once

#include <string>
#include <vector>
#include <optional>

// Forward declarations
struct command_t;
struct lexer_t;

struct variable_t {
    std::string name;
    std::string value;

    variable_t(std::string name, std::string value)
        : name(std::move(name))
        , value(std::move(value))
        {}
};

enum class expr_result_type {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NIL,
    RT_ERROR
};

struct expr_result_t {

    // TODO: unionize these fields. Right now, getting implicitly deleted destructor error because of union
    // and I don't want to block myself figuring it out right now.
    std::string str;
    std::string err;
    int exit_code;

    expr_result_type type;
};

class shell_t {

public:
    shell_t();

    std::optional<variable_t> find_variable(const std::string& name) const;

    int  execute_statement(const std::string& stmt);

    expr_result_t evaluate_export(lexer_t *lexer);

    expr_result_t evaluate_expression(const std::string& expr);

    int execute(const command_t *cmd);

    std::string substitute_variables(const std::string& stmt);

private:
    std::vector<variable_t> vars_;
};

int shell_loop(int argc, char **argv);

