#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <readline/readline.h>

#include <def.h>
#include <log.h>
#include <clean.h>


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

bool execute(char *const args[])
{
    const pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            error("could not find %s\n", args[0]);
            exit(EXIT_FAILURE);
        }

        return true;
    }

    // parent process
    int status;
    const pid_t child_pid = wait(&status);
    if (child_pid == -1) {
        // error waiting on child
        perror("wait");
        return false;
    } 
    
    if (WIFEXITED(status)) {
        debug("child %d exited with status %d\n", child_pid, WEXITSTATUS(status));
        return WEXITSTATUS(status) == EXIT_SUCCESS;
    } else if (WIFSIGNALED(status)) {
        debug("child %d terminated with signal %d and dumped?: %d\n", child_pid, WTERMSIG(status), WCOREDUMP(status));
    } else if (WIFSTOPPED(status)) {
        debug("child %d stopped with signal %d\n", child_pid, WSTOPSIG(status));
    }

    return true;
}

int shell_loop(int argc, char **argv)
{
    CLEAN_UNUSED(argc);
    CLEAN_UNUSED(argv);

    while (true) {
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