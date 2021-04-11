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

    static expr_result_t nil() {
        expr_result_t e;
        e.type = expr_result_type::RT_NIL;

        return e;
    }
};

class shell_t {
public:
    shell_t();

    // see GRAMMAR.txt for the language grammar spec
    int execute_statement(const std::string& stmt);

    int execute(const command_t& cmd);

    expr_result_t evaluate_expression(lexer_t& lexer);
    expr_result_t evaluate_export(lexer_t& lexer);

    std::optional<variable_t> find_variable(const std::string& name) const;

    std::string substitute_variables(std::string stmt, size_t beginning_at = 0);

    bool addenv(const std::string& name, const std::string& value) const;

private:
    std::vector<variable_t> vars_;
};

int shell_loop(int argc, char **argv);

