#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <readline/readline.h>

#include <clean/def.h>
#include <clean/log.h>
#include <clean/clean.h>
#include <clean/command.h>

static const char *CLEAN_BUILTIN_COMMANDS[] = {
    "quit",
    "cd"
};

static const size_t CLEAN_BUILTIN_COMMANDS_LENGTH = sizeof(CLEAN_BUILTIN_COMMANDS) / sizeof(CLEAN_BUILTIN_COMMANDS[0]);

static bool is_builtin_command(const command_t *cmd)
{
    for (size_t i = 0; i < CLEAN_BUILTIN_COMMANDS_LENGTH; ++i) {
        if (strcmp(CLEAN_BUILTIN_COMMANDS[i], cmd->cmd) == 0) {
            return true;
        }
    }

    return false;
}

// TODO: think about making a function table instead of this approach?
// Also, think about verifying arguments, # of arguments for each builtin??
static int execute_builtin_command(const command_t *cmd)
{
    if (strcmp("quit", cmd->cmd) == 0) {
        exit(EXIT_SUCCESS);
    }

    if (strcmp("cd", cmd->cmd) == 0) {
        if (chdir(cmd->args[1]) == -1) {
            perror("chdir");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

int execute(const command_t *cmd)
{
    // before executing a process on the PATH, check if the command is built in
    if (is_builtin_command(cmd)) {
        return execute_builtin_command(cmd);
    }

    const pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // child process
        if (execvp(cmd->cmd, cmd->args) == -1) {
            error("could not find %s\n", cmd->cmd);
            exit(EXIT_FAILURE);
        }

        return EXIT_SUCCESS;
    }

    // parent process
    int status;
    const pid_t child_pid = wait(&status);
    if (child_pid == -1) {
        // error waiting on child
        perror("wait");
        return EXIT_FAILURE;
    } 
    
    if (WIFEXITED(status)) {
        debug("child %d exited with status %d\n", child_pid, WEXITSTATUS(status));
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        debug("child %d terminated with signal %d and dumped?: %d\n", child_pid, WTERMSIG(status), WCOREDUMP(status));
    } else if (WIFSTOPPED(status)) {
        debug("child %d stopped with signal %d\n", child_pid, WSTOPSIG(status));
    }

    return EXIT_SUCCESS;
}

int shell_loop(int argc, char **argv)
{
    CLEAN_UNUSED(argc);
    CLEAN_UNUSED(argv);

    // when our shell exits, we want to ensure it exits
    // with an exit code equivalent to its last process
    int prev_process_exit_code = EXIT_SUCCESS;
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
            for (size_t i = 0; i < cmd.args_len; ++i) {
                debug("%s\n", cmd.args[i]);
            }

            prev_process_exit_code = execute(&cmd);
            
            command_destroy(&cmd);
            free(line);
        }
    }

    return prev_process_exit_code;
}