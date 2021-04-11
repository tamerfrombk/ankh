#pragma once

#include <string>

#include <clean/token.h>

class lexer_t {
public:
    lexer_t(std::string text);

    token_t next_token() noexcept;
    token_t peek_token() noexcept;

    bool is_eof() const noexcept;

    std::string rest() const noexcept;

private:
    void skip_whitespace() noexcept;
    token_t lex_alnum() noexcept;
    token_t lex_string() noexcept;

private:
    const std::string text_;
    size_t cursor_;
};