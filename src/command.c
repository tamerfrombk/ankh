#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <clean/command.h>
#include <clean/log.h>

static char **split_by_whitespace(const char *str)
{
    int tokens_capacity = 10;
    char **tokens = calloc(tokens_capacity, sizeof(*tokens));
    ASSERT_NOT_NULL(tokens);

    int tokens_len = 0;
    for (char c = *str; c != '\0'; c = *str) {
        if (isspace(c)) {
            ++str;
            continue;
        }

        if (tokens_len >= tokens_capacity) {
            debug("more than %d arguments supplied to the command line... resizing\n", tokens_capacity);
            
            const size_t new_capacity = tokens_capacity * 1.5;
            char **new_buf = realloc(tokens, sizeof(*tokens) * new_capacity);
            ASSERT_NOT_NULL(new_buf);

            tokens = new_buf;
            tokens_capacity = new_capacity;
        }

        // find the end of the current word
        int len = 0;
        for (const char *eow = str; *eow != '\0' && !isspace(*eow); ++eow) {
            ++len;
        }

        char *token = strndup(str, len);
        ASSERT_NOT_NULL(token);

        str += len;

        tokens[tokens_len++] = token;
    }

    return tokens;
}

void command_destroy(command_t *cmd)
{
    for (size_t i = 0; i < cmd->args_len; ++i) {
        free(cmd->args[i]);
    }
    free(cmd->args);

    cmd->cmd = NULL;
    cmd->args_len = 0;
}

void parse_command(const char *line, command_t *cmd)
{
    char **tokens = split_by_whitespace(line);

    cmd->cmd = tokens[0];
    cmd->args = tokens;

    cmd->args_len = 0;
    for (char **t = cmd->args; *t != NULL; ++t) {
        ++cmd->args_len;
    }
}
