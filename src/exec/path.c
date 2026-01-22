#define _POSIX_C_SOURCE 200809L  // for strtok_r

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

char* path_lookup(const char* command) {
    if (!command || !*command) return NULL;

    char* PATH = getenv("PATH");
    if (PATH == NULL) return NULL;
    char* path = strdup(PATH);

    char* saveptr = NULL;
    char* dir = strtok_r(path, PATH_LIST_SEPARATOR, &saveptr);

    while (dir) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);

        if (access(fullpath, X_OK) == 0) {
            char* result = strdup(fullpath);
            free(path);
            return result;
        }

        dir = strtok_r(NULL, PATH_LIST_SEPARATOR, &saveptr);
    }

    free(path);
    return NULL;
}