#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <readline/readline.h>

#include <def.h>
#include <log.h>
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

int execute(char *const args[])
{
    const pid_t pid = fork();
    if (pid == -1) {
        // error
        return -1;
    } else if (pid == 0) {
        // child
        if (execvp(args[0], args) == -1) {
            // TODO: hard error on failed process?
            exit(EXIT_FAILURE);
        }
        return 1;
    } else {
        // parent

        // TODO: inspect status of wait
        wait(NULL);
        return 1;
    }
}

int shell_loop(int argc, char **argv)
{
    CLEAN_UNUSED(argc);
    CLEAN_UNUSED(argv);

    while (!done) {
        char *line = readline("> ");
        if (line == NULL) {
            // EOF encountered on empty line
            debug("EOF\n");
            exit(EXIT_SUCCESS);
        } else if (*line == '\0') {
            // empty line
            debug("empty line\n");
            free(line);
        } else {
            debug("read line: '%s'\n", line);
            
            char **tokens = split_by_whitespace(line);
            
            debug("read line parsed:\n");
            for (char **t = tokens; *t != NULL; ++t) {
                debug("%s\n", *t);
            }

            execute(tokens);
            
            free(tokens);
            free(line);
        }
    }

    return 0;
}