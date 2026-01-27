#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shell.h"

int exec_pwd(const Command* cmd) {
    char* buffer = getcwd(NULL, 0);
    if (buffer == NULL) {
        perror("getcwd");
        return -1;
    }

    printf("%s\n", buffer);
    free(buffer);
    return 0;
}