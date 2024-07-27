#include <cctype>
#include <unordered_map>

#include <ankh/lang/exceptions.hpp>
#include <ankh/lang/lexer.hpp>

#include <ankh/log.hpp>

static const std::unordered_map<std::string, ankh::lang::TokenType> KEYWORDS = {
    {"true", ankh::lang::TokenType::ANKH_TRUE},
    {"false", ankh::lang::TokenType::ANKH_FALSE},
    {"nil", ankh::lang::TokenType::NIL},
    {"if", ankh::lang::TokenType::IF},
    {"else", ankh::lang::TokenType::ELSE},
    {"while", ankh::lang::TokenType::WHILE},
    {"for", ankh::lang::TokenType::FOR},
    {"break", ankh::lang::TokenType::BREAK},
    {"fn", ankh::lang::TokenType::FN},
    {"let", ankh::lang::TokenType::LET},
    {"return", ankh::lang::TokenType::ANKH_RETURN}};

ankh::lang::Lexer::Lexer(std::string text) : text_(std::move(text)), cursor_(0), line_(1), col_(1) {}

ankh::lang::Token ankh::lang::Lexer::next() {
    skip_whitespace();

    if (is_eof()) {
        // We avoid using tokenize() because we don't need line and col
        // calculations on the sentinel EOF token
        return {"EOF", TokenType::ANKH_EOF, line_, col_};
    }

    const char c = advance();
    if (c == '_' || std::isalpha(c)) {
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
    } else if (c == '[') {
        return tokenize("[", TokenType::LBRACKET);
    } else if (c == ']') {
        return tokenize("]", TokenType::RBRACKET);
    } else if (c == '&') {
        if (curr() == '&') {
            advance(); // eat the '&'
            return tokenize("&&", TokenType::AND);
        }
        panic<ScanException>(tokenize(curr(), TokenType::UNKNOWN), "'&' is not a valid token; did you mean '&&' ?");
    } else if (c == '|') {
        if (curr() == '|') {
            advance(); // eat the '|'
            return tokenize("||", TokenType::OR);
        }
        panic<ScanException>(tokenize(curr(), TokenType::UNKNOWN), "'|' is not a valid token; did you mean '||' ?");
    } else if (c == ';') {
        return tokenize(";", TokenType::SEMICOLON);
    } else if (c == ',') {
        return tokenize(",", TokenType::COMMA);
    } else if (c == ':') {
        return tokenize(":", TokenType::COLON);
    } else if (c == '$') {
        return scan_command();
    } else if (c == '.') {
        return tokenize(c, TokenType::DOT);
    } else {
        panic<ScanException>(tokenize(curr(), TokenType::UNKNOWN), "unknown token or token initializer: '{}'", c);
    }
}

ankh::lang::Token ankh::lang::Lexer::peek() noexcept {
    size_t old_cursor = cursor_;

    ankh::lang::Token token = next();

    cursor_ = old_cursor;

    return token;
}

bool ankh::lang::Lexer::is_eof() const noexcept { return cursor_ >= text_.length(); }

void ankh::lang::Lexer::skip_whitespace() noexcept {
    while (!is_eof() && std::isspace(curr())) {
        if (curr() == '\n') {
            ++line_;
            col_ = 0;
        }
        advance();
    }
}

void ankh::lang::Lexer::skip_comment() noexcept {
    while (!is_eof() && curr() != '\n') {
        advance();
    }
}

ankh::lang::Token ankh::lang::Lexer::scan_alnum() noexcept {
    std::string token(1, prev());
    while (!is_eof()) {
        char c = curr();
        if (c == '_' || std::isalnum(c)) {
            token += c;
            advance();
        } else {
            break;
        }
    }

    const TokenType type = is_keyword(token) ? KEYWORDS.at(token) : TokenType::IDENTIFIER;

    return tokenize(token, type);
}

ankh::lang::Token ankh::lang::Lexer::scan_string() {
    std::string str;

    size_t n_meta = 0;
    while (!is_eof()) {
        const char c = advance();
        if (is_eof()) {
            panic<ScanException>(tokenize(c, TokenType::UNKNOWN), "terminal \" not found");
        } else if (c == '\\') {
            const char n = advance();
            if (n == '"') {
                str += n;
            } else {
                str += c;
                str += n;
                ++n_meta;
            }
        } else if (c == '"') {
            break;
        } else {
            str += c;
        }
    }

    // -2:        to account for the quotes
    // + n_meta:  to account for extra characters added by meta characters
    return {str, TokenType::STRING, line_, col_ - str.length() - 2 + n_meta};
}

ankh::lang::Token ankh::lang::Lexer::scan_number() {
    std::string num(1, prev());

    bool decimal_found = false;
    while (!is_eof()) {
        char c = curr();
        if (std::isdigit(c)) {
            num += c;
            advance();
        } else if (c == '.') {
            if (decimal_found) {
                panic<ScanException>(tokenize(c, TokenType::UNKNOWN), "'.' lexeme not expected");
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

ankh::lang::Token ankh::lang::Lexer::scan_compound_operator(char expected, TokenType then,
                                                            TokenType otherwise) noexcept {
    const std::string before(1, prev());
    if (curr() == expected) {
        advance(); // eat it
        return tokenize(before + expected, then);
    }

    return tokenize(before, otherwise);
}

ankh::lang::Token ankh::lang::Lexer::scan_command() {
    if (curr() != '(') {
        panic<ScanException>(tokenize(curr(), TokenType::UNKNOWN), "'(' token is expected after '$' for command");
    }

    advance(); // eat the '('

    std::string value;
    while (!is_eof()) {
        char c = advance();
        if (c == ')') {
            break;
        } else if (is_eof()) {
            panic<ScanException>(tokenize(c, TokenType::UNKNOWN), "terminal ')' not found");
        } else {
            value += c;
        }
    }

    return tokenize(value, TokenType::COMMAND);
}

char ankh::lang::Lexer::prev() const noexcept { return text_[cursor_ - 1]; }

char ankh::lang::Lexer::curr() const noexcept { return text_[cursor_]; }

char ankh::lang::Lexer::peekc() const noexcept { return text_[cursor_ + 1]; }

char ankh::lang::Lexer::advance() noexcept {
    char c = text_[cursor_++];

    ++col_;

    return c;
}

ankh::lang::Token ankh::lang::Lexer::tokenize(char c, TokenType type) const noexcept {
    return tokenize(std::string(1, c), type);
}

ankh::lang::Token ankh::lang::Lexer::tokenize(const std::string &s, TokenType type) const noexcept {
    return {s, type, line_, col_ - s.length()};
}

bool ankh::lang::is_keyword(const std::string &str) noexcept { return KEYWORDS.find(str) != KEYWORDS.cend(); }

std::vector<ankh::lang::Token> ankh::lang::scan(const std::string &source) {
    // The new line is added here so that that while loop below will continue one last iteration
    // after the last character in the actual source and emit a EOF token.
    // It didn't need to be a new line; any whitespace character would have worked as well
    ankh::lang::Lexer lexer(source + "\n");

    std::vector<ankh::lang::Token> tokens;
    while (!lexer.is_eof()) {
        tokens.push_back(lexer.next());
    }

#ifndef NDEBUG
    // for (const auto& tok : tokens) {
    //     // ANKH_DEBUG("{}", tok);
    // }
#endif

    return tokens;
}
