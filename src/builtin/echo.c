#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

int exec_echo(const Command* command) {
    for (int i = 1; i < command->argc; i++) {
        write(STDOUT_FILENO, command->argv[i], strlen(command->argv[i]));
        if (i + 1 < command->argc) write(STDOUT_FILENO, " ", 1);
    }

    write(STDOUT_FILENO, "\n", 1);

    return 0;
}
