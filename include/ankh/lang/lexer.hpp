#pragma once

#include <string>
#include <vector>

#include <ankh/lang/token.hpp>

namespace ankh::lang {

class Lexer {
public:
    Lexer(std::string text);

    Token next();
    Token peek() noexcept;

    bool is_eof() const noexcept;

private:
    void skip_whitespace() noexcept;
    void skip_comment() noexcept;
    
    Token scan_alnum() noexcept;
    Token scan_string();
    Token scan_number();
    Token scan_compound_operator(char expected, TokenType then, TokenType otherwise) noexcept;
    Token scan_command();

    char prev() const noexcept;
    char curr() const noexcept;
    char peekc() const noexcept;
    char advance() noexcept;

    Token tokenize(char c, TokenType type) const noexcept;
    Token tokenize(const std::string& s, TokenType type) const noexcept;

private:
    const std::string text_;
    size_t cursor_;
    size_t line_;
    size_t col_;
};

bool is_keyword(const std::string& str) noexcept;

std::vector<Token> scan(const std::string& source);

}