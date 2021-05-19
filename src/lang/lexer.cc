#include <cctype>
#include <unordered_map>

#include <fak/lang/lexer.h>
#include <fak/lang/error_handler.h>

static const std::unordered_map<std::string, fk::lang::token_type> KEYWORDS = {
      { "true", fk::lang::token_type::BTRUE }
    , { "false", fk::lang::token_type::BFALSE }
    , { "nil", fk::lang::token_type::NIL }
    , { "print", fk::lang::token_type::PRINT }
    , { "if", fk::lang::token_type::IF }
    , { "else", fk::lang::token_type::ELSE }
    , { "while", fk::lang::token_type::WHILE }
    , { "for", fk::lang::token_type::FOR }
    , { "let", fk::lang::token_type::LET }
};

fk::lang::lexer::lexer(std::string text, fk::lang::error_handler *error_handler)
    : text_(std::move(text))
    , cursor_(0)
    , line_(1)
    , error_handler_(error_handler)
{}

fk::lang::token fk::lang::lexer::next_token() noexcept
{
    skip_whitespace();

    if (is_eof()) {
        return { "EOF", fk::lang::token_type::T_EOF, line_, 0 };
    }

    char c = advance();
    if (std::isalpha(c)) {
        return lex_alnum(c);
    } else if (std::isdigit(c)) {
        return lex_number(c);  
    } else if (c == '+') {
        return { "+", fk::lang::token_type::PLUS, line_, 0 };
    } else if (c == '-') {
        return { "-", fk::lang::token_type::MINUS, line_, 0 };
    } else if (c == '*') {
        return { "*", fk::lang::token_type::STAR, line_, 0 };
    } else if (c == '/') {
        return { "/", fk::lang::token_type::FSLASH, line_, 0 };
    } else if (c == '(') {
        return { "(", fk::lang::token_type::LPAREN, line_, 0 };  
    } else if (c == ')') {
        return { ")", fk::lang::token_type::RPAREN, line_, 0 };
    } else if (c == '<') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"<=", fk::lang::token_type::LTE, line_, 0 };
        }
        return { "<", fk::lang::token_type::LT, line_, 0 };
    } else if (c == '>') {
        if (curr() == '=') {
            advance(); // eat the '='
            return { ">=", fk::lang::token_type::GTE, line_, 0 };
        }
        return { ">", fk::lang::token_type::GT, line_, 0 };
    } else if (c == '=') {
        if (curr() == '=') {
            advance(); // eat the other equal
            return { "==", fk::lang::token_type::EQEQ, line_, 0 };
        }
        return { "=", fk::lang::token_type::EQ, line_, 0 };
    } else if (c == '!') {
        if (curr() == '=') {
            advance(); // eat the '='
            return { "!=", fk::lang::token_type::NEQ, line_, 0 };
        }
        return { "!", fk::lang::token_type::BANG, line_, 0 };
    } else if (c == '"') {
        return lex_string();
    } else if (c == '#') {
        skip_comment();
        return next_token();
    } else if (c == '{') {
        return { "{", fk::lang::token_type::LBRACE, line_, 0 };
    } else if (c == '}') {
        return { "}", fk::lang::token_type::RBRACE, line_, 0 };
    } else if (c == '&') {
        if (curr() == '&') {
            advance(); // eat the '&'
            return { "&&", fk::lang::token_type::AND, line_, 0 };
        }
        error_handler_->report_error({"'&' is not a valid token; did you mean '&&'?"});
        return { "&", fk::lang::token_type::UNKNOWN, line_, 0 };
    } else if (c == '|') {
        if (curr() == '|') {
            advance(); // eat the '|'
            return { "||", fk::lang::token_type::OR, line_, 0 };
        }
        error_handler_->report_error({"'|' is not a valid token; did you mean '||'?"});
        return { "|", fk::lang::token_type::UNKNOWN, line_, 0 };
    } else if (c == ';') {
        return { ";", fk::lang::token_type::SEMICOLON, line_, 0 };
    } else {
        error_handler_->report_error({"unknown token!"});
        return { std::string{c}, fk::lang::token_type::UNKNOWN, line_, 0 };
    }
}

fk::lang::token fk::lang::lexer::peek_token() noexcept
{
    size_t old_cursor = cursor_;
    
    fk::lang::token token = next_token();

    cursor_ = old_cursor;

    return token;
}

bool fk::lang::lexer::is_eof() const noexcept
{
    return cursor_ >= text_.length();
}

void fk::lang::lexer::skip_whitespace() noexcept
{
    while (!is_eof() && std::isspace(curr())) {
        if (curr() == '\n') {
            ++line_;
        }
        advance();
    }
}

void fk::lang::lexer::skip_comment() noexcept
{
    while (!is_eof() && curr() != '\n') {
        advance();
    }
}

fk::lang::token fk::lang::lexer::lex_alnum(char init) noexcept
{
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

    fk::lang::token_type type = is_keyword(token)
        ? KEYWORDS.at(token)
        : fk::lang::token_type::IDENTIFIER;

    return { token, type, line_, 0 };
}

fk::lang::token fk::lang::lexer::lex_string() noexcept
{
    std::string str;
    while (!is_eof()) {
        char c = advance();
        if (c == '"') {
            break;
        } else if (is_eof()) {
            error_handler_->report_error({"terminal \" not found"});
            return { str, fk::lang::token_type::UNKNOWN, line_, 0 };
        } else {
            str += c;
        }
    }

    return { str, fk::lang::token_type::STRING, line_, 0 };
}

fk::lang::token fk::lang::lexer::lex_number(char init) noexcept
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
                return { ".", fk::lang::token_type::UNKNOWN, line_, 0 };
            }
            num += c;
            decimal_found = true;
            advance();
        } else {
            break;
        }
    }

    return { num, fk::lang::token_type::NUMBER, line_, 0 };
}

char fk::lang::lexer::curr() const noexcept
{
    return text_[cursor_];
}

char fk::lang::lexer::peek() const noexcept
{
    return text_[cursor_ + 1];
}

char fk::lang::lexer::advance() noexcept
{
    return text_[cursor_++];
}

bool fk::lang::lexer::is_keyword(const std::string& str) const noexcept
{
    return KEYWORDS.find(str) != KEYWORDS.cend();
}