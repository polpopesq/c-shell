#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtin/builtin.h"
#include "exec.h"
#include "redirection.h"

int exec_builtin(builtin_func bf, const ParsedCommand* command) {
    int saved_fds[3];
    if (apply_redirections(command, saved_fds) != 0) {
        perror("apply_redirections");
        return -1;
    } else {
        int result = bf(command);
        restore_fds(saved_fds);
        return result;
    }
}

int exec_external(const ParsedCommand* command) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    // in the child process(0), no need to save and restore fds
    if (pid == 0) {
        if (apply_redirections(command, NULL) != 0) {
            _exit(1);
        }
        execvp(command->argv[0], command->argv);
        // only gets past this point if it fails.
        fprintf(stderr, "%s: command not found\n", command->argv[0]);
        _exit(127);
    }
    // parent
    int child_status;
    pid_t child = waitpid(pid, &child_status, 0);
    if (WIFEXITED(child_status)) {
        int code = WEXITSTATUS(child_status);
        return code;  // return child's exit code to caller
    } else if (WIFSIGNALED(child_status)) {
        int sig = WTERMSIG(child_status);
        fprintf(stderr, "Child killed by signal %d\n", sig);
        return 128 + sig;  // common shell convention
    }

    return -1;  // unknown termination
}

int execute_command(const ParsedCommand* cmd) {
    builtin_func bf = find_builtin(cmd->argv[0]);
    if (bf) {
        return exec_builtin(bf, cmd);
    } else {
        return exec_external(cmd);
    }
}