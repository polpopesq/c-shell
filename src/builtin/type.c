#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtin/builtin.h"
#include "exec/path.h"
#include "shell.h"

int exec_type(const ParsedCommand* cmd) {
    if (cmd->argc < 2 || cmd->argv[1] == NULL) {
        return 2;
    }
    if (find_builtin(cmd->argv[1]) == NULL) {
        char* result = path_lookup(cmd->argv[1]);
        if (!result) {
            printf("%s: not found\n", cmd->argv[1]);
        } else {
            printf("%s is %s\n", cmd->argv[1], result);
            free(result);
        }
    } else {
        printf("%s is a shell builtin\n", cmd->argv[1]);
    }
    return 0;
}