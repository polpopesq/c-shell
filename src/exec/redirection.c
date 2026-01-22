#include "redirection.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

void restore_fds(int saved_fds[3]) {
    dup2(saved_fds[0], STDIN_FILENO);
    dup2(saved_fds[1], STDOUT_FILENO);
    dup2(saved_fds[2], STDERR_FILENO);

    close(saved_fds[0]);
    close(saved_fds[1]);
    close(saved_fds[2]);
}

int apply_redirections(const ParsedCommand* cmd, int saved_fds[3]) {
    if (saved_fds != NULL) {
        saved_fds[0] = dup(STDIN_FILENO);
        saved_fds[1] = dup(STDOUT_FILENO);
        saved_fds[2] = dup(STDERR_FILENO);

        if (saved_fds[0] < 0 || saved_fds[1] < 0 || saved_fds[2] < 0) {
            perror("dup");
            return -1;
        }
    }

    for (int i = 0; i < cmd->redirc; ++i) {
        const Redirection* r = &cmd->redirections[i];
        int flags = O_WRONLY | O_CREAT;
        if (r->mode == TRUNC) {
            flags |= O_TRUNC;
        } else {
            flags |= O_APPEND;
        }

        int fd = open(r->filename, flags, 0644);
        if (fd < 0) {
            perror(r->filename);
            if (saved_fds != NULL) restore_fds(saved_fds);
            return -1;
        }
        if (dup2(fd, r->target_fd) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}