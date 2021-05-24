#pragma once

#include <string>

namespace fk::lang {

enum class token_type {
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
    // "B" for "bool" -- prepended to avoid clashing with TRUE/FALSE macros
    BTRUE,             // "true"
    BFALSE,            // "false"
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
    // prepended FK because RETURN is a macro in libreadline
    // TODO: once we remove libreadline, we can rename this back to just RETURN
    FK_RETURN,         // "return"
    WALRUS,            // :=
    NUMBER,
    STRING,
    T_EOF,
    UNKNOWN
};

std::string token_type_str(token_type type);

struct token {
    std::string str;
    token_type type;
    size_t line;
    size_t inline_pos;

    token(std::string str, token_type type, size_t line, size_t inlinepos)
        : str(std::move(str)), type(type), line(line), inline_pos(inlinepos) {}

};

inline bool operator==(const token& lhs, const token& rhs)
{
    // this order is done on purpose to avoid an O(n) string comparison if we can help it
    return lhs.type == rhs.type
        && lhs.line == rhs.line
        && lhs.inline_pos == rhs.inline_pos
        && lhs.str == rhs.str;
}

inline bool operator!=(const token& lhs, const token& rhs)
{
    return !operator==(lhs, rhs);
}

}
