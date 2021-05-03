#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <memory>
#include <utility>
#include <optional>

#include <readline/readline.h>

#include <fak/def.h>
#include <fak/log.h>
#include <fak/fak.h>
#include <fak/lang/lexer.h>
#include <fak/lang/parser.h>

#include <fak/internal/pretty_printer.h>
#include <fak/lang/error_handler.h>
#include <fak/lang/interpreter.h>

static int interpret(const std::string& script)
{
    auto error_handler = std::make_unique<fk::lang::error_handler>();
    fk::lang::parser parser(script, error_handler.get());

    const fk::lang::program program = parser.parse();
    if (error_handler->error_count() > 0) {
        const auto& errors = error_handler->errors();
        for (const auto& e : errors) {
            std::fprintf(stderr, "%s\n", e.msg.c_str());
        }
        return 1;
    }

    fk::lang::interpreter interpreter;
    interpreter.interpret(program);

    return 0;
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

int fk::shell_loop(int argc, char **argv)
{
    if (argc > 1) {
        if (auto possible_script = read_file(argv[1]); possible_script) {
            return interpret(possible_script.value());
        }
        
        fk::log::error("could not open script '%s'\n", argv[1]);

        return EXIT_FAILURE;
    }

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
    while (true) {
        char *line = readline("> ");
        if (line == nullptr) {
            // EOF encountered on empty line
            fk::log::debug("EOF\n");
            exit(EXIT_SUCCESS);
        } else if (*line == '\0') {
            // empty line
            fk::log::debug("empty line\n");
            free(line);
        } else {
            fk::log::debug("read line: '%s'\n", line);
            prev_process_exit_code = interpret(line);
            free(line);
        }
    }

    return prev_process_exit_code;
}