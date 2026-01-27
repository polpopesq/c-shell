#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int exec_exit(const Command* cmd) {
    int code = 0;  // default exit code

    // If user provides an argument: exit <code>
    if (cmd->argc > 1) {
        char* endptr = NULL;
        code = strtol(cmd->argv[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "exit: numeric argument required\n");
            code = 1;
        }
    }

    // Optional: cleanup before exiting
    // free_parsed_command(cmd);  // if you want to free immediately
    // free any other global resources if needed

    exit(code);  // terminate shell
}