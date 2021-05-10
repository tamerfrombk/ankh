#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <readline/readline.h>

#include <fak/log.h>
#include <fak/fak.h>

#include <fak/lang/parser.h>
#include <fak/lang/error_handler.h>
#include <fak/lang/interpreter.h>
#include <fak/lang/interpretation_exception.h>

#include <fak/internal/pretty_printer.h>

#include <fmt/color.h>

static void print_error(const char *msg)
{
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "{}\n", msg);
}

static void print_error(const std::string& msg)
{
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "{}\n", msg);
}

static int execute(fk::lang::interpreter& interpreter, const std::string& script) noexcept
{
    auto error_handler = std::make_unique<fk::lang::error_handler>();
    fk::lang::parser parser(script, error_handler.get());

    const fk::lang::program program = parser.parse();
    if (error_handler->error_count() > 0) {
        const auto& errors = error_handler->errors();
        for (const auto& e : errors) {
           print_error(e.msg);
        }
        return EXIT_FAILURE;
    }

    try {
        interpreter.interpret(program);
    } catch (const fk::lang::interpretation_exception& e) {
        print_error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
    fk::lang::interpreter interpreter;

    if (argc > 1) {
        if (auto possible_script = read_file(argv[1]); possible_script) {
            return execute(interpreter, possible_script.value());
        }
        
        fk::log::error("could not open script '%s'\n", argv[1]);

        return EXIT_FAILURE;
    }

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
    while (true) {
        // TODO: remove dependency on libreadline
        char *line = readline("> ");
        if (line == nullptr) {
            fk::log::debug("EOF\n");
            return EXIT_SUCCESS;
        } else if (*line == '\0') {
            fk::log::debug("empty line\n");
        } else {
            fk::log::debug("read line: '%s'\n", line);
            prev_process_exit_code = execute(interpreter, line);
        }
        free(line);
    }

    return prev_process_exit_code;
}