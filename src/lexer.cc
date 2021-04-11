#include "clean/token.h"
#include <cctype>

#include <clean/log.h>
#include <clean/lexer.h>

lexer_t::lexer_t(std::string text)
    : text_(std::move(text))
    , cursor_(0)
{}

token_t lexer_t::next_token() noexcept
{
    skip_whitespace();

    if (is_eof()) {
        debug("EOF reached\n");
        return { "", token_type::T_EOF };
    }

    char c = text_[cursor_];
    if (c == '=') {
        ++cursor_; // eat the =
        return { "=", token_type::EQ };
    } else if (isalpha(c)) {
        return lex_alnum();
    } else if (c == '"') {
        ++cursor_; // eat the ""
        return lex_string();
    } else {
        return { "", token_type::UNKNOWN };
    }
}

token_t lexer_t::peek_token() noexcept
{
    size_t old_cursor = cursor_;
    
    token_t token = next_token();

    cursor_ = old_cursor;

    return token;
}

bool lexer_t::is_eof() const noexcept
{
    return cursor_ >= text_.length();
}

std::string lexer_t::rest() const noexcept
{
    return is_eof()
        ? ""
        : text_.substr(cursor_);   
}

void lexer_t::skip_whitespace() noexcept
{
    debug("LEXER: skipping whitespace\n");
    while (!is_eof() && std::isspace(text_[cursor_])) {
        ++cursor_;
    }
}

token_t lexer_t::lex_alnum() noexcept
{
    debug("LEXER: lexing alnum token\n");

    std::string token;
    while (!is_eof()) {
        char c = text_[cursor_];
        if (std::isalnum(c)) {
            token += c;
            ++cursor_;
        } else {
            break;
        }
    }

    token_type type = token_type::IDENTIFIER;
    // TODO: create keyword table
    if (token == "export") {
        type = token_type::KEYWORD;
    }

    return { token, type };
}

token_t lexer_t::lex_string() noexcept
{
    std::string str;
    while (!is_eof()) {
        char c = text_[cursor_++];
        if (c == '"') {
            break;
        } else if (is_eof()) {
            error("terminal \" not found\n");
            return { str, token_type::UNKNOWN };
        } else {
            str += c;
        }
    }

    return { str, token_type::STRING };
}