#include <stdio.h>

#include "shell.h"

int exec_echo(const ParsedCommand* command) {
    for (int i = 1; i < command->argc; i++) {
        printf("%s ", command->argv[i]);
    }
    printf("\n");
    return 0;
}