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

class shell_t {
public:
    shell_t();

    // see GRAMMAR.txt for the language grammar spec
    int execute_statement(const std::string& stmt);
    int execute_script(const std::string& script);
    int execute(const command_t& cmd);

    std::optional<variable_t> find_variable(const std::string& name) const;

    std::string substitute_variables(std::string stmt, size_t beginning_at = 0);

    bool addenv(const std::string& name, const std::string& value) const;

private:
    std::vector<variable_t> vars_;
};

int shell_loop(int argc, char **argv);

