#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>

#include <def.h>
#include <clean.h>

// TODO: handle signal to gracefully kill shell
volatile int done = 0;

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
            free(line);
        }
    }

    return 0;
}