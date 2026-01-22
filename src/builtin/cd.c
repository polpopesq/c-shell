#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

int exec_cd(const ParsedCommand* cmd) {
    const char* target = cmd->argv[1];
    if (target == NULL || (strcmp(target, "~") == 0)) {
        target = getenv("HOME");
    }

    if (chdir(target) != 0) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "cd: %s", target);
        perror(msg);
        return -1;
    }
    return 0;
}