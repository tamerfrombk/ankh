#pragma once

#include <string>

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
    // "B" for "bool" -- prepended to avoid clashing with TRUE/FALSE macros
    BTRUE,             // "true"
    BFALSE,            // "false"
    NIL,               // "nil"
    PRINT,             // "print"
    NUMBER,
    STRING,
    T_EOF,
    UNKNOWN
};

std::string token_type_str(token_type type);

struct token_t {
    std::string str;
    token_type type;

    token_t(std::string str, token_type type);
};