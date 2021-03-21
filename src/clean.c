#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <readline/readline.h>

#include <def.h>
#include <clean.h>

// TODO: handle signal to gracefully kill shell
volatile int done = 0;

// TODO: move this somewhere we can test it
char **split_by_whitespace(const char *str)
{
    // TODO: expand number of arguments or customize them
    char **tokens = calloc(10, sizeof(*tokens));
    if (tokens == NULL) {
        // TODO: OOM error
    }

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
        if (token == NULL) {
            // TODO: OOM error
        }

        str += len;

        tokens[t++] = token;
    }

    return tokens;
}

int shell_loop(int argc, char **argv)
{
    CLEAN_UNUSED(argc);
    CLEAN_UNUSED(argv);

    while (!done) {
        char *line = readline("> ");
        if (line == NULL) {
            // EOF encountered on empty line
            puts("EOF");
        } else if (*line == '\0') {
            // empty line
            puts("empty line");
            free(line);
        } else {
            printf("you entered '%s'\n", line);
            char **tokens = split_by_whitespace(line);
            for (char **t = tokens; *t != NULL; ++t) {
                puts(*t);
                free(*t);
            }
            free(tokens);
            free(line);
        }
    }

    return 0;
}