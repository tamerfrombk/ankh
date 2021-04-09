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

static const char *CLEAN_BUILTIN_COMMANDS[] = {
    "quit",
    "cd"
};

static const size_t CLEAN_BUILTIN_COMMANDS_LENGTH = sizeof(CLEAN_BUILTIN_COMMANDS) / sizeof(CLEAN_BUILTIN_COMMANDS[0]);

static bool is_builtin_command(const command_t *cmd)
{
    for (size_t i = 0; i < CLEAN_BUILTIN_COMMANDS_LENGTH; ++i) {
        if (cmd->cmd == CLEAN_BUILTIN_COMMANDS[i]) {
            return true;
        }
    }

    return false;
}

// TODO: think about making a function table instead of this approach?
// Also, think about verifying arguments, # of arguments for each builtin??
static int execute_builtin_command(shell_t *sh, const command_t *cmd)
{
    CLEAN_UNUSED(sh);
    
    if (cmd->cmd == "quit") {
        exit(EXIT_SUCCESS);
    }

    if (cmd->cmd == "cd") {
        if (chdir(cmd->args[1].c_str()) == -1) {
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
    if (value == nullptr) {
        return std::nullopt;
    }

    variable_t v { name, value };

    return std::optional(v);
}

int shell_t::execute(const command_t *cmd)
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
        char **vs = new char*[cmd->args.size()];
        int i = 0;
        for (auto& arg : cmd->args) {
            vs[i] = const_cast<char*>(arg.c_str());
            debug("vs: '%s'\n", vs[i]);
            ++i;
        }

        if (execvp(cmd->cmd.c_str(), vs) == -1) {
            error("could not find '%s'\n", cmd->cmd.c_str());
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

std::string shell_t::substitute_variables(const std::string& stmt)
{
    std::vector<std::string> words = split_by_whitespace(stmt);

    debug("Before variable evaluation %s:\n", stmt.c_str());

    for (size_t i = 0; i < words.size(); ++i) {
        std::string& word = words[i];
        debug("WORD: '%s'\n", word.c_str());
        // it might be a variable so let's be sure
        if (word[0] == '$' && word.length() > 1) {
            // it is a variable
            auto var = find_variable(word.substr(1));
            if (var.has_value()) {
                // substitute the current word with the expanded variable value
                auto v = var.value();
                debug("variable '%s' = '%s'\n", v.name.c_str(), v.value.c_str());
                word = v.value;
            } else {
                auto v = word.substr(1);
                error("%s is not defined in this scope\n", v.c_str());
            }
        }
    }

    debug("After variable evaluation:\n");
    for (const auto& word : words) {
        debug("token: %s\n", word.c_str());
    }

    // concat all the strings back together now
    return join(words, " ");
}

expr_result_t shell_t::evaluate_export(lexer_t *lexer)
{
    token_t identifier = lexer->next_token();
    if (identifier.type != token_type::IDENTIFIER) {
        auto str = token_type_str(identifier.type);
        error("expected identifier but got %s\n", str.c_str());
    }

    token_t eq = lexer->next_token();
    if (eq.type != token_type::EQ) {
        auto str = token_type_str(eq.type);
        error("expected eq but got %s\n", str.c_str());
    }

    token_t str = lexer->next_token();
    if (str.type != token_type::STRING) {
        auto msg = token_type_str(str.type);
        error("expected str but got %s\n", msg.c_str());
    }

    // add to the environment and overwrite old values
    if (setenv(identifier.str.c_str(), str.str.c_str(), 1) == -1) {
        perror("setenv");
    }

    expr_result_t result;
    result.type = expr_result_type::RT_NIL;

    return result;
}

int shell_t::execute_statement(const std::string& stmt)
{
    const expr_result_t result = evaluate_expression(stmt);
    if (result.type == expr_result_type::RT_ERROR) {
        error("unable to evaluate expression due to :%s\n", result.err.c_str());
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

// statement := <variable_definition>
// variable_definition := export <identifier> | export <identifier> = <string>
// string := "[a-zA-Z0-9 ]+"
expr_result_t shell_t::evaluate_expression(const std::string& expr)
{
    std::string substituted_line = substitute_variables(expr);

    debug("beginning lexical analysis on '%s'\n", substituted_line.c_str());

    lexer_t lexer(substituted_line);

    token_t tok = lexer.next_token();
    if (tok.type == token_type::T_EOF) {
        debug("EOF evaluated\n");
        
        expr_result_t result;
        result.type = expr_result_type::RT_NIL;

        return result;
    } else if (tok.type == token_type::KEYWORD) {
        if (tok.str == "export") {
            return evaluate_export(&lexer);
        } else {
            fatal("KEYWORD can't happen\n");
        }
    } else { // assume command
        command_t cmd = parse_command(substituted_line);

        int exit_code = execute(&cmd);

        expr_result_t result;
        result.type = expr_result_type::RT_EXIT_CODE;
        result.exit_code = exit_code;

        return result;
    }
}

int shell_loop(int argc, char **argv)
{
    CLEAN_UNUSED(argc);
    CLEAN_UNUSED(argv);

    shell_t shell;

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
    while (true) {
        char *line = readline("> ");
        if (line == NULL) {
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