#pragma once

#include <string>
#include <vector>

#include <fak/lang/token.h>

namespace fk::lang {

// forward declarations
struct ErrorHandler;

class Lexer {
public:
    Lexer(std::string text, ErrorHandler *error_handler);

    Token next() noexcept;
    Token peek() noexcept;

    bool is_eof() const noexcept;
    bool is_keyword(const std::string& str) const noexcept;

private:
    void skip_whitespace() noexcept;
    void skip_comment() noexcept;
    
    Token scan_alnum() noexcept;
    Token scan_string() noexcept;
    Token scan_number() noexcept;
    Token scan_compound_operator(char expected, TokenType then, TokenType otherwise) noexcept;

    char prev() const noexcept;
    char curr() const noexcept;
    char peek() const noexcept;
    char advance() noexcept;

    Token tokenize(char c, TokenType type) const noexcept;
    Token tokenize(const std::string& s, TokenType type) const noexcept;

private:
    const std::string text_;
    size_t cursor_;
    size_t line_;
    size_t col_;

    ErrorHandler *error_handler_;
};

std::vector<Token> scan(const std::string& source, ErrorHandler *error_handler) noexcept;

}