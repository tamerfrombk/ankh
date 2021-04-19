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

    char c = advance();
    if (c == '=') {
        return { "=", token_type::EQ };
    } else if (std::isalpha(c)) {
        return lex_alnum(c);
    } else if (std::isdigit(c)) {
        return lex_number(c);  
    } else if (c == '+') {
        return { "+", token_type::PLUS };
    } else if (c == '-') {
        return { "-", token_type::MINUS };
    } else if (c == '*') {
        return { "*", token_type::STAR };
    } else if (c == '/') {
        return { "/", token_type::FSLASH };
    } else if (c == '(') {
        return { "(", token_type::LPAREN };  
    } else if (c == ')') {
        return { ")", token_type::RPAREN };
    } else if (c == '=') {
        if (curr() == '=') {
            advance(); // eat the other equal
            return {"==", token_type::EQEQ};
        }
        return {"=", token_type::EQ};
    } else if (c == '<') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"<=", token_type::LTE};
        }
        return {"<", token_type::LT};
    } else if (c == '>') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {">=", token_type::GTE};
        }
        return {">", token_type::GT};
    } else if (c == '!') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"!=", token_type::NEQ};
        }
        return {"!", token_type::BANG};
    } else if (c == '"') {
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

token_t lexer_t::lex_alnum(char init) noexcept
{
    debug("LEXER: lexing alnum token\n");

    std::string token(1, init);
    while (!is_eof()) {
        char c = curr();
        if (std::isalnum(c)) {
            token += c;
            advance();
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
        char c = advance();
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

// TODO: lex floating point numbers
token_t lexer_t::lex_number(char init) noexcept
{
    std::string num(1, init);
    while (!is_eof()) {
        char c = curr();
        if (std::isdigit(c)) {
            num += c;
            advance();
        } else {
            break;
        }
    }

    return { num, token_type::NUMBER };
}

char lexer_t::curr() const noexcept
{
    return text_[cursor_];
}

char lexer_t::peek() const noexcept
{
    return text_[cursor_ + 1];
}

char lexer_t::advance() noexcept
{
    return text_[cursor_++];
}