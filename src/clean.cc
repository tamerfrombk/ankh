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
#include <iostream>
#include <optional>

#include <readline/readline.h>

#include <clean/def.h>
#include <clean/log.h>
#include <clean/clean.h>
#include <clean/string.h>
#include <clean/lang/lexer.h>
#include <clean/lang/parser.h>

#include <clean/internal/pretty_printer.h>
#include <clean/lang/error_handler.h>
#include <clean/lang/interpreter.h>

static void print_result(const expr_result_t& result)
{
    switch (result.type) {
    case expr_result_type::RT_ERROR:
        error("error evaluating expression: '%s'\n", result.err.c_str());
        break;
    case expr_result_type::RT_STRING:
        std::puts(result.str.c_str());
        break;
    case expr_result_type::RT_NUMBER:
        std::printf("%f\n", result.n);
        break;
    case expr_result_type::RT_BOOL:
        std::puts(result.b ? "true" : "false");
        break;
    case expr_result_type::RT_NIL:
        std::puts("nil");
        break;
    default:
        error("unhandled expression result type");
        break;
    }
}

static int interpret(const std::string& script)
{
    auto errh = std::make_unique<error_handler_t>();
    parser_t parser(script, errh.get());

    pretty_printer_t printer;
    interpreter_t interpreter;
    while (!parser.is_eof()) {
        expression_ptr expr = parser.parse_expression();
        if (errh->error_count() > 0) {
            const auto& errors = errh->errors();
            for (const auto& e :  errors) {
                std::cerr << "ERROR! : " << e.msg << "\n";
            }
            return 1;
        }
        std::string pp = expr->accept(&printer).str;
        expr_result_t result = expr->accept(&interpreter);
        print_result(result);
    }
    
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

int shell_loop(int argc, char **argv)
{
    if (argc > 1) {
        if (auto possible_script = read_file(argv[1]); possible_script) {
            return interpret(possible_script.value());
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
            prev_process_exit_code = interpret(line);
            free(line);
        }
    }

    return prev_process_exit_code;
}