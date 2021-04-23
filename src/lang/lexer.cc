#include "clean/lang/token.h"
#include <cctype>
#include <unordered_map>

#include <clean/log.h>
#include <clean/lang/lexer.h>
#include <clean/lang/error_handler.h>

static const std::unordered_map<std::string, token_type> KEYWORDS = {
      { "true", token_type::BTRUE }
    , { "false", token_type::BFALSE }
    , { "nil", token_type::NIL }
    , { "print", token_type::PRINT }
};

lexer_t::lexer_t(std::string text, error_handler_t *error_handler)
    : text_(std::move(text))
    , cursor_(0)
    , error_handler_(error_handler)
{}

token_t lexer_t::next_token() noexcept
{
    skip_whitespace();

    if (is_eof()) {
        debug("EOF reached\n");
        return { "", token_type::T_EOF };
    }

    char c = advance();
    if (std::isalpha(c)) {
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
    } else if (c == '=') {
        if (curr() == '=') {
            advance(); // eat the other equal
            return {"==", token_type::EQEQ};
        }
        return {"=", token_type::EQ};
    } else if (c == '!') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"!=", token_type::NEQ};
        }
        return {"!", token_type::BANG};
    } else if (c == '"') {
        return lex_string();
    } else if (c == '#') {
        skip_comment();
        return next_token();
    } else {
        error_handler_->report_error({"unknown token!"});
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
        advance();
    }
}

void lexer_t::skip_comment() noexcept
{
    while (!is_eof() && curr() != '\n') {
        advance();
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

    token_type type = is_keyword(token)
        ? KEYWORDS.at(token)
        : token_type::IDENTIFIER;

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
            error_handler_->report_error({"terminal \" not found"});
            return { str, token_type::UNKNOWN };
        } else {
            str += c;
        }
    }

    return { str, token_type::STRING };
}

token_t lexer_t::lex_number(char init) noexcept
{
    std::string num(1, init);

    bool decimal_found = false;
    while (!is_eof()) {
        char c = curr();
        if (std::isdigit(c)) {
            num += c;
            advance();
        } else if (c == '.') {
            if (decimal_found) {
                error_handler_->report_error({"'.' lexeme not expected"});
                return { ".", token_type::UNKNOWN };
            }
            num += c;
            decimal_found = true;
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

bool lexer_t::is_keyword(const std::string& str) const noexcept
{
    return KEYWORDS.find(str) != KEYWORDS.cend();
}