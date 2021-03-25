#pragma once

#include <stddef.h>

typedef enum token_type {
    KEYWORD,
    IDENTIFIER,
    EQ,
    STRING,
    T_EOF,
    UNKNOWN
} token_type;

char *token_type_str(token_type type);

typedef struct token_t {
    const char *str;
    size_t len;
    token_type type;
} token_t;

token_t *make_token(const char *str, size_t len, token_type type);