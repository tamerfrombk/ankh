#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <clean/command.h>
#include <clean/log.h>

static char **split_by_whitespace(const char *str)
{
    // TODO: expand number of arguments or customize them
    char **tokens = calloc(10, sizeof(*tokens));
    ASSERT_NOT_NULL(tokens);

    int t = 0;
    for (char c = *str; c != '\0'; c = *str) {
        if (isspace(c)) {
            ++str;
            continue;
        }

        // find the end of the current word
        int len = 0;
        for (const char *eow = str; *eow != '\0' && !isspace(*eow); ++eow) {
            ++len;
        }

        char *token = strndup(str, len);
        ASSERT_NOT_NULL(token);

        str += len;

        tokens[t++] = token;
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
