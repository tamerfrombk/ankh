#pragma once

#include <string>
#include <iostream>

#include <fmt/core.h>

namespace fk::lang {

enum class TokenType {
    IDENTIFIER,
    EQ,                // =
    EQEQ,              // ==
    BANG,              // !
    NEQ,               // !=
    LT,                // <
    LTE,               // <=
    GT,                // >
    GTE,               // >=
    MINUS,             // -
    MINUSEQ,           // -=
    PLUS,              // +
    PLUSEQ,            // +=
    FSLASH,            // /
    FSLASHEQ,          // /=
    STAR,              // *
    STAREQ,            // *=
    LPAREN,            // (
    RPAREN,            // )
    LBRACE,            // {
    RBRACE,            // }
    // FK prefix is added to avoid clashing with the TRUE/FALSE macros defined in libc
    FK_TRUE,           // "true"
    FK_FALSE,          // "false"
    NIL,               // "nil"
    PRINT,             // "print"
    IF,                // "if"
    ELSE,              // "else"
    AND,               // &&
    OR,                // ||
    WHILE,             // "while"
    FOR,               // "for"
    SEMICOLON,         // ;
    LET,               // "let"
    COMMA,             // ,
    DEF,               // "def"
    // prepended FK because RETURN is a macro defined in some library
    FK_RETURN,         // "return"
    WALRUS,            // :=
    INC,               // ++
    DEC,               // --
    NUMBER,
    STRING,
    FK_EOF,
    UNKNOWN
};

std::string token_type_str(TokenType type) noexcept;

struct Token {
    std::string str;
    TokenType type;
    size_t line;
    size_t col;

    Token(std::string str, TokenType type, size_t line, size_t col)
        : str(std::move(str)), type(type), line(line), col(col) {}
};

inline bool operator==(const Token& lhs, const Token& rhs) noexcept
{
    // this order is done on purpose to avoid an O(n) string comparison if we can help it
    return lhs.type == rhs.type
        && lhs.line == rhs.line
        && lhs.col == rhs.col
        && lhs.str == rhs.str;
}

inline bool operator!=(const Token& lhs, const Token& rhs) noexcept
{
    return !operator==(lhs, rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Token& t) noexcept
{
    return os << "(" << token_type_str(t.type) << ", " << t.str << ", " << t.line << ", " << t.col << ")";
}

}
