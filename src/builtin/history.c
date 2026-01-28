#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

void print_history(int limit) {
    HIST_ENTRY** list = history_list();  // get all history entries
    if (!list) {
        printf("No history.\n");
        return;
    }

    int total = history_length;
    int start = 0;
    if (limit > 0 && limit < total) {
        start = total - limit;  // last 'limit' entries
    }

    for (int i = start; i < total; ++i) {
        HIST_ENTRY* entry = list[i];  // direct array access
        if (entry && entry->line) {
            printf("%5d  %s\n", i + history_base, entry->line);
        }
    }
}

typedef int (*history_op_func)(const char* filename);

// NOT thread-safe!
int history_read_op(const char* filename) { return read_history(filename); }
int history_write_op(const char* filename) { return write_history(filename); }
static int last_append_index = 0;
int history_append_op(const char* filename) {
    int new_entries = history_length - last_append_index;
    if (new_entries <= 0) return 0;

    int result = append_history(new_entries, filename);
    if (result == 0) {
        last_append_index = history_length;
    }
    return result;
}

typedef struct {
    char mode;  // 'r', 'w', 'a'
    history_op_func func;
} history_op_entry;

static history_op_entry history_ops[] = {
    {'r', history_read_op},
    {'w', history_write_op},
    {'a', history_append_op},
};

int exec_history(const Command* c) {
    if (c->argc > 2) {
        if (c->argv[1][0] != '-') {
            fprintf(stderr, "history: invalid second argument '%s'\n",
                    c->argv[1]);
            return 1;
        } else {
            char mode = c->argv[1][1];
            for (size_t i = 0; i < sizeof(history_ops) / sizeof(history_ops[0]);
                 i++) {
                if (history_ops[i].mode == mode) {
                    int result = history_ops[i].func(c->argv[2]);
                    if (result != 0) {
                        fprintf(stderr, "history: couldn't %c %s\n", mode,
                                c->argv[2]);
                    }
                    return result;
                }
            }
            fprintf(stderr, "history: invalid mode '%c'\n", mode);
            return 1;
        }

    } else {                    // just print history
        int history_limit = 0;  // 0 means print all
        if (c->argc == 2) {
            char* endptr;
            history_limit = strtol(c->argv[1], &endptr, 10);
            if (*endptr != '\0' || history_limit < 0) {
                printf("Invalid history limit: %s\n", c->argv[1]);
                return 1;
            }
        }

        print_history(history_limit);
    }
    return 0;
}

void initialize_history() {
    const char* HISTFILE_PATH = getenv("HISTFILE");
    read_history(HISTFILE_PATH);
}