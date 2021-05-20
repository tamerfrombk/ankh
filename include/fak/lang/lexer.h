#pragma once

#include <string>

#include <fak/lang/token.h>

namespace fk::lang {

// forward declarations
struct error_handler;

class lexer {
public:
    lexer(std::string text, error_handler *error_handler);

    token next_token() noexcept;
    token peek_token() noexcept;

    bool is_eof() const noexcept;

private:
    void skip_whitespace() noexcept;
    void skip_comment() noexcept;
    
    token lex_alnum(char init) noexcept;
    token lex_string() noexcept;
    token lex_number(char init) noexcept;
    token lex_compound_operator(char expected, token_type then, token_type otherwise) noexcept;

    char prev() const noexcept;
    char curr() const noexcept;
    char peek() const noexcept;
    char advance() noexcept;

    bool is_keyword(const std::string& str) const noexcept;

private:
    const std::string text_;
    size_t cursor_;
    size_t line_;

    error_handler *error_handler_;
};

}