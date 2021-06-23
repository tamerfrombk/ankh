#include <cctype>
#include <unordered_map>

#include <fak/lang/lexer.h>
#include <fak/lang/exceptions.h>

#include <fak/log.h>

static const std::unordered_map<std::string, fk::lang::TokenType> KEYWORDS = {
      { "true", fk::lang::TokenType::FK_TRUE }
    , { "false", fk::lang::TokenType::FK_FALSE }
    , { "nil", fk::lang::TokenType::NIL }
    , { "print", fk::lang::TokenType::PRINT }
    , { "if", fk::lang::TokenType::IF }
    , { "else", fk::lang::TokenType::ELSE }
    , { "while", fk::lang::TokenType::WHILE }
    , { "for", fk::lang::TokenType::FOR }
    , { "def", fk::lang::TokenType::DEF }
    , { "return", fk::lang::TokenType::FK_RETURN }
};

fk::lang::Lexer::Lexer(std::string text)
    : text_(std::move(text))
    , cursor_(0)
    , line_(1)
    , col_(1)
{}

fk::lang::Token fk::lang::Lexer::next()
{
    skip_whitespace();

    if (is_eof()) {
        // We avoid using tokenize() because we don't need line and col
        // calculations on the sentinel EOF token
        return { "EOF", TokenType::FK_EOF, line_, col_ };
    }

    const char c = advance();
    if (std::isalpha(c)) {
        return scan_alnum();
    } else if (std::isdigit(c)) {
        return scan_number();  
    } else if (c == '+') {
        if (curr() == '+') {
            advance(); // eat it
            return tokenize("++", TokenType::INC);
        }
        return scan_compound_operator('=', TokenType::PLUSEQ, TokenType::PLUS);
    } else if (c == '-') {
        if (curr() == '-') {
            advance(); // eat it
            return tokenize("--", TokenType::DEC);
        }
        return scan_compound_operator('=', TokenType::MINUSEQ, TokenType::MINUS);
    } else if (c == '*') {
        return scan_compound_operator('=', TokenType::STAREQ, TokenType::STAR);
    } else if (c == '/') {
        return scan_compound_operator('=', TokenType::FSLASHEQ, TokenType::FSLASH);
    } else if (c == '(') {
        return tokenize("(", TokenType::LPAREN);
    } else if (c == ')') {
        return tokenize(")", TokenType::RPAREN);
    } else if (c == '<') {
        return scan_compound_operator('=', TokenType::LTE, TokenType::LT);
    } else if (c == '>') {
        return scan_compound_operator('=', TokenType::GTE, TokenType::GT);
    } else if (c == '=') {
        return scan_compound_operator('=', TokenType::EQEQ, TokenType::EQ);
    } else if (c == '!') {
        return scan_compound_operator('=', TokenType::NEQ, TokenType::BANG);
    } else if (c == '"') {
        return scan_string();
    } else if (c == '#') {
        skip_comment();
        return next();
    } else if (c == '{') {
        return tokenize("{", TokenType::LBRACE);
    } else if (c == '}') {
        return tokenize("}", TokenType::RBRACE);
    } else if (c == '&') {
        if (curr() == '&') {
            advance(); // eat the '&'
            return tokenize("&&", TokenType::AND);
        }
        panic<ScanException>("'&' is not a valid token; did you mean '&&' ?");
    } else if (c == '|') {
        if (curr() == '|') {
            advance(); // eat the '|'
            return tokenize("||", TokenType::OR);
        }
        panic<ScanException>("'|' is not a valid token; did you mean '||' ?");
    } else if (c == ';') {
        return tokenize(";", TokenType::SEMICOLON);
    } else if (c == ',') {
        return tokenize(",", TokenType::COMMA);
    } else if (c == ':') {
        if (curr() == '=') {
            advance(); // eat the '='
            return tokenize(":=", TokenType::WALRUS);
        }
        panic<ScanException>("':' is not a valid token; did you mean ':=' ?");
    } else {
        panic<ScanException>("unknown token!");
    }
}

fk::lang::Token fk::lang::Lexer::peek() noexcept
{
    size_t old_cursor = cursor_;
    
    fk::lang::Token token = next();

    cursor_ = old_cursor;

    return token;
}

bool fk::lang::Lexer::is_eof() const noexcept
{
    return cursor_ >= text_.length();
}

void fk::lang::Lexer::skip_whitespace() noexcept
{
    while (!is_eof() && std::isspace(curr())) {
        if (curr() == '\n') {
            ++line_;
            col_ = 0;
        }
        advance();
    }
}

void fk::lang::Lexer::skip_comment() noexcept
{
    while (!is_eof() && curr() != '\n') {
        advance();
    }
}

fk::lang::Token fk::lang::Lexer::scan_alnum() noexcept
{
    std::string token(1, prev());
    while (!is_eof()) {
        char c = curr();
        if (std::isalnum(c)) {
            token += c;
            advance();
        } else {
            break;
        }
    }

    const TokenType type = is_keyword(token)
        ? KEYWORDS.at(token)
        : TokenType::IDENTIFIER;

    return tokenize(token, type);
}

fk::lang::Token fk::lang::Lexer::scan_string()
{
    // TODO: handle escape sequences within strings
    std::string str;
    while (!is_eof()) {
        char c = advance();
        if (c == '"') {
            break;
        } else if (is_eof()) {
            panic<ScanException>("terminal \" not found");
        } else {
            str += c;
        }
    }

    return tokenize(str, TokenType::STRING);
}

fk::lang::Token fk::lang::Lexer::scan_number()
{
    std::string num(1, prev());

    bool decimal_found = false;
    while (!is_eof()) {
        char c = curr();
        if (std::isdigit(c)) {
            num += c;
            advance();
        } else if (c == '.') {
            if (decimal_found) {
                panic<ScanException>("'.' lexeme not expected");
            }
            num += c;
            decimal_found = true;
            advance();
        } else {
            break;
        }
    }

    return tokenize(num, TokenType::NUMBER);
}

fk::lang::Token fk::lang::Lexer::scan_compound_operator(char expected, TokenType then, TokenType otherwise) noexcept
{
    const std::string before(1, prev());
    if (curr() == expected) {
        advance(); // eat it
        return tokenize(before + expected, then);
    }

    return tokenize(before, otherwise);
}

char fk::lang::Lexer::prev() const noexcept
{
    return text_[cursor_ - 1];
}

char fk::lang::Lexer::curr() const noexcept
{
    return text_[cursor_];
}

char fk::lang::Lexer::peek() const noexcept
{
    return text_[cursor_ + 1];
}

char fk::lang::Lexer::advance() noexcept
{
    char c = text_[cursor_++];

    ++col_;

    return c;
}

fk::lang::Token fk::lang::Lexer::tokenize(char c, TokenType type) const noexcept
{
    return tokenize(std::string(1, c), type);
}

fk::lang::Token fk::lang::Lexer::tokenize(const std::string& s, TokenType type) const noexcept
{
    // When we initialize a string token, we have to take into account both of the double quotes surrounding
    // the actual string and remove that from the reported column position.
    // For example, if we have the following string token that begins at line 1, col 1:
    // "i am a string"
    // ^-- line = 1, col = 1
    // then after we're done scanning the token, we'll be here:
    // "i am a string"
    //                ^-- line = 1, col = 16
    // If we simply subtract the length of the string without the double quotes, we'll end up at column 3
    // which is incorrect.
    const size_t column = type == TokenType::STRING
        ? col_ - s.length() - 2
        : col_ - s.length();

    return { s, type, line_, column };
}

bool fk::lang::is_keyword(const std::string& str) noexcept
{
    return KEYWORDS.find(str) != KEYWORDS.cend();
}

std::vector<fk::lang::Token> fk::lang::scan(const std::string& source)
{
    // The new line is added here so that that while loop below will continue one last iteration
    // after the last character in the actual source and emit a EOF token.
    // It didn't need to be a new line; any whitespace character would have worked as well
    fk::lang::Lexer lexer(source + "\n");

    std::vector<fk::lang::Token> tokens;
    while (!lexer.is_eof()) {
        tokens.push_back(lexer.next());
    }

    for (const auto& tok : tokens) {
        FK_DEBUG("{}", tok);
    }

    return tokens;
}