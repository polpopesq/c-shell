#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int exec_history(const Command* c) {
    int history_limit = 0;  // 0 means print all
    if (c->argc > 1) {
        char* endptr;
        history_limit = strtol(c->argv[1], &endptr, 10);
        if (*endptr != '\0' || history_limit < 0) {
            printf("Invalid history limit: %s\n", c->argv[1]);
            return 1;
        }
    }

    int total = history_length;  // total entries in history
    if (total == 0) {
        printf("No history.\n");
        return 0;
    }

    int start = 0;
    if (history_limit > 0 && history_limit < total) {
        start = total - history_limit;  // start from last n entries
    }

    for (int i = start; i < total; ++i) {
        HIST_ENTRY* entry =
            history_get(i + history_base);  // get entry directly
        if (entry) {
            printf("%5d  %s\n", i + history_base, entry->line);
        }
    }

    return 0;
}
