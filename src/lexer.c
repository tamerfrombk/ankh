#include <string.h>
#include <ctype.h>

#include <clean/def.h>
#include <clean/log.h>
#include <clean/lexer.h>

void lexer_init(lexer_t *lexer, const char *text, size_t len)
{
    lexer->text = text;
    lexer->text_len = len;
    lexer->cursor = 0;
}

void lexer_teardown(lexer_t *lexer)
{
    lexer->text = NULL;
    lexer->text_len = 0;
    lexer->cursor = 0;
}

static void lex_skip_whitespace(lexer_t *lexer)
{
    debug("LEXER: skipping whitespace\n");
    while (!lex_is_at_end(lexer) && isspace(lexer->text[lexer->cursor])) {
        ++lexer->cursor;
    }
}

static token_t *lex_alnum(lexer_t *lexer)
{
    debug("LEXER: lexing alnum token\n");

    char token[CLEAN_LEXER_MAX_TOKEN_LENGTH + 1] = {0};
    size_t token_len = 0;

    while (!lex_is_at_end(lexer)) {
        char c = lexer->text[lexer->cursor];
        if (isalnum(c)) {
            token[token_len++] = c;
            if (token_len >= CLEAN_LEXER_MAX_TOKEN_LENGTH) {
                fatal("max token length reached\n");
            }
            ++lexer->cursor;
        } else {
            break;
        }
    }

    token_type type = IDENTIFIER;
    // TODO: create keyword table
    if (strcmp(token, "export") == 0) {
        type = KEYWORD;
    }

    char *value = strdup(token);
    ASSERT_NOT_NULL(value);

    return make_token(value, token_len, type);
}

static token_t *lex_string(lexer_t *lexer)
{
    size_t capacity = 32;
    char *buf = calloc(capacity, sizeof(*buf));
    ASSERT_NOT_NULL(buf);

    size_t len = 0;
    while (!lex_is_at_end(lexer)) {
        char c = lexer->text[lexer->cursor++];
        if (c == '"') {
            break;
        } else if (lex_is_at_end(lexer)) {
            error("terminal \" not found\n");
            free(buf);
            return NULL;
        } else {
            if (len + 1 >= capacity) {
                int new_capacity = capacity * 2;
                char *new_buf = realloc(buf, capacity * 2);
                ASSERT_NOT_NULL(new_buf);

                buf = new_buf;
                capacity = new_capacity;
            }
            buf[len++] = c;
        }
    }

    return make_token(buf, len, STRING);
}

token_t *lex_next_token(lexer_t *lexer)
{
    lex_skip_whitespace(lexer);

    if (lex_is_at_end(lexer)) {
        debug("EOF reached\n");
        return make_token(NULL, 0, T_EOF);
    }

    char c = lexer->text[lexer->cursor];
    if (c == '=') {
        char *value = strdup("=");
        ASSERT_NOT_NULL(value);

        ++lexer->cursor; // eat the =

        return make_token(value, 1, EQ);
    } else if (isalpha(c)) {
        return lex_alnum(lexer);
    } else if (c == '"') {
        ++lexer->cursor; // eat the ""
        return lex_string(lexer);
    } else {
        return make_token(NULL, 0, UNKNOWN);
    }
}

bool lex_is_at_end(const lexer_t *lexer)
{
    return lexer->cursor >= lexer->text_len;
}