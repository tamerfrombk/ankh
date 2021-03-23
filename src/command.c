#include <stdlib.h>

#include <clean/command.h>
#include <clean/string.h>

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
    char **words = NULL;
    size_t num_words = 0;
    split_by_whitespace(line, &words, &num_words);

    cmd->cmd = words[0];
    cmd->args = words;

    cmd->args_len = num_words;
    for (char **t = cmd->args; *t != NULL; ++t) {
        ++cmd->args_len;
    }
}
