#include <stdlib.h>
#include <string.h>

#include <clean/token.h>
#include <clean/log.h>

char *token_type_str(token_type type)
{
    switch (type) {
    case KEYWORD:
        return strdup("KEYWORD");
    case IDENTIFIER:
        return strdup("IDENTIFIER");
    case EQ:
        return strdup("EQ");
    case STRING:
        return strdup("STRING");
    case T_EOF:
        return strdup("EOF");
    case UNKNOWN:
        return strdup("UNKNOWN");
    default:
        fatal("unknown type\n");
    }
}

token_t *make_token(const char *str, size_t len, token_type type)
{
    token_t *token = calloc(1, sizeof(*token));
    ASSERT_NOT_NULL(token);

    token->str = str;
    token->len = len;
    token->type = type;

    return token;
}