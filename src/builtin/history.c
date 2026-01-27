#include <readline/history.h>
#include <stdio.h>

#include "shell.h"

int exec_history(const Command* c) {
    HIST_ENTRY** cmd_history_entries = history_list();

    if (!cmd_history_entries) {
        printf("No history.\n");
        return 0;
    }

    for (int i = 0; cmd_history_entries[i] != NULL; ++i) {
        printf("%5d  %s\n", i + history_base, cmd_history_entries[i]->line);
    }

    return 0;
}