#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <clean/token.h>
typedef struct lexer_t {
    const char *text;
    size_t text_len;
    size_t cursor;
} lexer_t;

void lexer_init(lexer_t *lexer, const char *text, size_t len);
void lexer_teardown(lexer_t *lexer);

token_t *lex_next_token(lexer_t *lexer);

bool lex_is_at_end(const lexer_t *lexer);