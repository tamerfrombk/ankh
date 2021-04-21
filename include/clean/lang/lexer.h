#pragma once

#include <string>

#include <clean/lang/token.h>

// forward declarations
struct error_handler_t;

class lexer_t {
public:
    lexer_t(std::string text, error_handler_t *error_handler);

    token_t next_token() noexcept;
    token_t peek_token() noexcept;

    bool is_eof() const noexcept;

    std::string rest() const noexcept;

private:
    void skip_whitespace() noexcept;
    token_t lex_alnum(char init) noexcept;
    token_t lex_string() noexcept;
    token_t lex_number(char init) noexcept;

    char curr() const noexcept;
    char peek() const noexcept;
    char advance() noexcept;

private:
    const std::string text_;
    size_t cursor_;

    error_handler_t *error_handler_;
};