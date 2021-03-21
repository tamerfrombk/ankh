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
#include <command.h>

bool execute(const command_t *cmd)
{
    const pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        // child process
        if (execvp(cmd->cmd, cmd->args) == -1) {
            error("could not find %s\n", cmd->cmd);
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
            
            command_t cmd;
            parse_command(line, &cmd);
            
            debug("read line parsed:\n");
            debug("command: %s\n", cmd.cmd);
            for (char **t = cmd.args; *t != NULL; ++t) {
                debug("%s\n", *t);
            }

            execute(&cmd);
            
            command_destroy(&cmd);
            free(line);
        }
    }

    return 0;
}