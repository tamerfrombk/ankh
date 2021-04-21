#pragma once

#include <string>

enum class token_type {
    KEYWORD,
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