#include "clean/token.h"
#include <cstddef>
#include <optional>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>

#include <readline/readline.h>

#include <clean/def.h>
#include <clean/log.h>
#include <clean/clean.h>
#include <clean/command.h>
#include <clean/string.h>
#include <clean/lexer.h>
#include <clean/parser.h>

#include <clean/internal/pretty_printer.h>

static const char *CLEAN_BUILTIN_COMMANDS[] = {
    "quit",
    "cd"
};

static const size_t CLEAN_BUILTIN_COMMANDS_LENGTH = sizeof(CLEAN_BUILTIN_COMMANDS) / sizeof(CLEAN_BUILTIN_COMMANDS[0]);

static bool is_builtin_command(const command_t& cmd)
{
    for (size_t i = 0; i < CLEAN_BUILTIN_COMMANDS_LENGTH; ++i) {
        if (cmd.cmd == CLEAN_BUILTIN_COMMANDS[i]) {
            return true;
        }
    }

    return false;
}

// TODO: think about making a function table instead of this approach?
// Also, think about verifying arguments, # of arguments for each builtin??
static int execute_builtin_command(shell_t *sh, const command_t& cmd)
{
    CLEAN_UNUSED(sh);
    
    if (cmd.cmd == "quit") {
        exit(EXIT_SUCCESS);
    }

    if (cmd.cmd == "cd") {
        if (chdir(cmd.args[1].c_str()) == -1) {
            perror("chdir");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

shell_t::shell_t()
{
    // TODO: determine if vars_ is required
    vars_.emplace_back("VARIABLE", "this is the expanded value of VARIABLE");
}

std::optional<variable_t> shell_t::find_variable(const std::string &name) const {
    char *value = std::getenv(name.c_str());
    debug("getenv('%s') = '%s'\n", name.c_str(), value);

    if (value == nullptr) {
        return std::nullopt;
    }

    variable_t v { name, value };

    return std::optional(v);
}

int shell_t::execute(const command_t& cmd)
{
    // before executing a process on the PATH, check if the command is built in
    if (is_builtin_command(cmd)) {
        return execute_builtin_command(this, cmd);
    }

    const pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // child process
        // add one to NULL terminate the array of arguments per execvp()'s man page spec
        char **vs = new char*[cmd.args.size() + 1];
        int i = 0;
        for (auto& arg : cmd.args) {
            vs[i] = const_cast<char*>(arg.c_str());
            debug("vs: '%s'\n", vs[i]);
            ++i;
        }
        vs[i] = NULL;

        if (execvp(cmd.cmd.c_str(), vs) == -1) {
            error("could not find '%s'\n", cmd.cmd.c_str());
            exit(EXIT_FAILURE);
        }
        free(vs);

        return EXIT_SUCCESS;
    }

    // parent process
    int status;
    const pid_t child_pid = wait(&status);
    if (child_pid == -1) {
        // error waiting on child
        perror("wait");
        return EXIT_FAILURE;
    } 
    
    if (WIFEXITED(status)) {
        debug("child %d exited with status %d\n", child_pid, WEXITSTATUS(status));
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        debug("child %d terminated with signal %d and dumped?: %d\n", child_pid, WTERMSIG(status), WCOREDUMP(status));
    } else if (WIFSTOPPED(status)) {
        debug("child %d stopped with signal %d\n", child_pid, WSTOPSIG(status));
    }

    return EXIT_SUCCESS;
}

int shell_t::execute_script(const std::string& script)
{
    parser_t parser(script);

    expression_ptr expr = parser.parse_expression();

    pretty_printer_t printer;
    std::string pp = expr->accept(&printer);

    debug("PP: %s\n", pp.c_str());

    return 0;
}

std::string shell_t::substitute_variables(std::string stmt, size_t beginning_at)
{
    const auto idx = stmt.find_first_of("$", beginning_at);
    if (idx == std::string::npos) {
        return stmt;
    }

    std::string var_name;
    for (size_t i = idx + 1; i < stmt.length() && std::isalnum(stmt[i]); ++i) {
        var_name += stmt[i];
    }

    // false alarm, just a "$"
    if (var_name.empty()) {
        return stmt;
    }

    if (auto potential_variable = find_variable(var_name); potential_variable.has_value()) {
            const variable_t& variable = potential_variable.value();
            auto prefix = stmt.substr(0, idx);
            // idx points to where the '$' is so we add 1 to remove it and the variable itself from the suffix
            auto suffix = stmt.substr(idx + 1 + variable.name.length());
            
            return substitute_variables(prefix + variable.value + suffix, idx + 1);
    } else {
        error("'%s' is not defined in the current scope\n", var_name.c_str());
    }
    
    return stmt;
}

bool shell_t::addenv(const std::string& name, const std::string& value) const
{
    if (setenv(name.c_str(), value.c_str(), 1) == -1) {
        return false;
    }

    return true;
}

expr_result_t shell_t::evaluate_export(lexer_t& lexer)
{
    debug("evaluating export expression: '%s'\n", lexer.rest().c_str());

    // "export"
    {
        const token_t tok = lexer.next_token();
        if (tok.str != "export") {
            auto str = token_type_str(tok.type);
            error("expected 'export' but got '%s' of type '%s'\n", tok.str.c_str(), str.c_str());
        }
    }


    // identifier
    const token_t identifier = lexer.next_token();
    if (identifier.type != token_type::IDENTIFIER) {
        auto str = token_type_str(identifier.type);
        error("expected 'IDENTIFIER' but got '%s' of type '%s'\n", identifier.str.c_str(), str.c_str());
    }

    // "="
    {
        const token_t eq = lexer.next_token();
        if (eq.type != token_type::EQ) {
            auto str = token_type_str(eq.type);
            error("expected '=' but got '%s' of type '%s'\n", eq.str.c_str(), str.c_str());
        }
    }

    // right hand expression
    const expr_result_t result = evaluate_expression(lexer);
    switch (result.type) {
    case expr_result_type::RT_ERROR:
        error("error evaluating expression: '%s'\n", result.err.c_str());
        return result;
    case expr_result_type::RT_NIL:
        addenv(identifier.str, "");
        return result;
    case expr_result_type::RT_STRING:
        addenv(identifier.str, result.str);
        return expr_result_t::nil();
    default:
        return expr_result_t::nil();
    }
}

int shell_t::execute_statement(const std::string& stmt)
{
    const std::string substituted_line = substitute_variables(stmt);

    debug("substituted line: '%s'\n", substituted_line.c_str());

    lexer_t lexer(substituted_line);

    const expr_result_t result = evaluate_expression(lexer);
    if (result.type == expr_result_type::RT_ERROR) {
        error("unable to evaluate expression due to: '%s'\n", result.err.c_str());
        return EXIT_FAILURE;
    } else if (result.type == expr_result_type::RT_NIL) {
        return EXIT_SUCCESS;
    } else if (result.type == expr_result_type::RT_EXIT_CODE) {
        return result.exit_code;
    } else {
        std::puts(result.str.c_str());
        return EXIT_SUCCESS;
    }
}

expr_result_t shell_t::evaluate_expression(lexer_t& lexer)
{
    debug("evaluating expression '%s'\n", lexer.rest().c_str());
    
    token_t peek = lexer.peek_token();

    if (peek.type == token_type::T_EOF) {
        debug("EOF evaluated\n");

        return expr_result_t::nil();
    }

    if (peek.str == "export") {
        return evaluate_export(lexer);
    }

    // now we advance the lexer

    token_t tok = lexer.next_token();
    if (tok.type == token_type::STRING) {

        expr_result_t result;
        result.type = expr_result_type::RT_STRING;
        result.str  = tok.str;

        return result;
    }


    // assume command
    auto rest = tok.str + lexer.rest();
    debug("rest: %s\n", rest.c_str());

    command_t cmd = parse_command(rest);
    int exit_code = execute(cmd);

    expr_result_t result;
    result.type = expr_result_type::RT_EXIT_CODE;
    result.exit_code = exit_code;

    return result;
}

static std::optional<std::string> read_file(const std::string& path) noexcept
{
    std::FILE *fp = std::fopen(path.c_str(), "r");
    if (fp == nullptr) {
        return std::nullopt;
    }

    // TODO: instead of reading one character at a time, read a whole bunch to increase performance
    std::string result;
    char c;
    while ((c = std::fgetc(fp)) != EOF) {
        result += c;
    }
    
    std::fclose(fp);

    return { result };
}

int shell_loop(int argc, char **argv)
{
    shell_t shell;

    if (argc > 1) {
        if (auto possible_script = read_file(argv[1]); possible_script) {
            return shell.execute_script(possible_script.value());
        }
        
        error("could not open script '%s'\n", argv[1]);

        return EXIT_FAILURE;
    }

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
    while (true) {
        char *line = readline("> ");
        if (line == nullptr) {
            // EOF encountered on empty line
            debug("EOF\n");
            exit(EXIT_SUCCESS);
        } else if (*line == '\0') {
            // empty line
            debug("empty line\n");
            free(line);
        } else {
            debug("read line: '%s'\n", line);
            prev_process_exit_code = shell.execute_statement(line);
            free(line);
        }
    }

    return prev_process_exit_code;
}