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
    PLUS,              // +
    FSLASH,            // /
    STAR,              // *
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
    NUMBER,
    STRING,
    T_EOF,
    UNKNOWN
};

std::string token_type_str(token_type type);

struct token {
    std::string str;
    token_type type;

    token(std::string str, token_type type);
};

}
