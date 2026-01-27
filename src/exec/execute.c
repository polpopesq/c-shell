#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtin/builtin.h"
#include "exec.h"
#include "redirection.h"

int exec_builtin(builtin_func bf, const Command* command) {
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

int exec_external(const Command* command) {
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

int execute_command(const Command* cmd) {
    builtin_func bf = find_builtin(cmd->argv[0]);
    if (bf) {
        return exec_builtin(bf, cmd);
    } else {
        return exec_external(cmd);
    }
}

// Sentinel meaning: "do not override, inherit from parent"
enum { FD_INHERIT = -1 };

// Conventional indices for pipe()
enum { PIPE_READ = 0, PIPE_WRITE = 1 };

int execute_pipeline(const Pipeline* pl) {
    // Holds the read end of the previous pipe.
    // FD_INHERIT means: use normal stdin.
    if (pl->count == 1) {
        return execute_command(&pl->cmds[0]);
    }

    int prev_read = FD_INHERIT;

    // Store all child PIDs so we can wait for them later
    pid_t pids[pl->count];

    // Iterate once per command in the pipeline
    for (int i = 0; i < pl->count; i++) {
        // pipefd[0] = read end, pipefd[1] = write end
        // Initialized to FD_INHERIT to mean "no pipe"
        int pipefd[2] = {FD_INHERIT, FD_INHERIT};

        // Create a pipe only if this is NOT the last command
        // Last command writes to stdout, not to a pipe
        if (i < pl->count - 1) {
            pipe(pipefd);
        }

        // Fork a new process for this command
        pid_t pid = fork();

        if (pid == 0) {
            // ======================
            // CHILD PROCESS
            // ======================

            // If there is a previous pipe, connect it to stdin
            // This makes:
            //   previous_command | current_command
            if (prev_read != FD_INHERIT) {
                dup2(prev_read, STDIN_FILENO);
            }

            // If there is a next pipe, connect stdout to it
            // This makes:
            //   current_command | next_command
            if (pipefd[PIPE_WRITE] != FD_INHERIT) {
                dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);
            }

            // Apply redirections (<, >, >>, etc.)
            // These OVERRIDE any pipe wiring if present
            apply_redirections(&pl->cmds[i], NULL);

            // Execute the command (builtin or external)
            execute_command(&pl->cmds[i]);

            // If exec fails, exit immediately
            _exit(1);
        }

        // ======================
        // PARENT PROCESS
        // ======================

        // Save PID for later waiting
        pids[i] = pid;

        // Parent must close fds it does not use
        // Otherwise pipes never reach EOF and hang

        // Close previous pipe read end (no longer needed)
        if (prev_read != FD_INHERIT) {
            close(prev_read);
        }

        // Close current pipe write end in parent
        if (pipefd[PIPE_WRITE] != FD_INHERIT) {
            close(pipefd[PIPE_WRITE]);
        }

        // Carry forward the read end for the next command
        // This becomes stdin for the next child
        prev_read = pipefd[PIPE_READ];
    }

    // ======================
    // WAIT FOR ALL CHILDREN
    // ======================

    int status, last = 0;

    // Wait for every command in the pipeline
    for (int i = 0; i < pl->count; i++) {
        waitpid(pids[i], &status, 0);

        // Shell convention:
        // pipeline exit status = exit status of last command
        if (i == pl->count - 1) last = WEXITSTATUS(status);
    }

    // Return status of final pipeline command
    return last;
}