#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>

#include "input.h"

static const char* builtin_candidates[] = {"echo", "cd",   "pwd",
                                           "type", "exit", NULL};

char* my_generator(const char* text, int state) {
    // static iteration index because generator is called multiple times
    static int i;
    size_t len;

    if (state == 0) {
        i = 0;  // reset for a new TAB completion attempt
    }

    len = strlen(text);

    while (builtin_candidates[i]) {
        const char* cand = builtin_candidates[i];
        i++;

        // prefix match: cand starts with text
        if (strncmp(cand, text, len) == 0) {
            return strdup(cand);  // readline expects malloc'd string
        }
    }

    return NULL;  // no more matches
}

char** my_completion(const char* text, int start, int end) {
    (void)end;

    if (start != 0) return NULL;  // fallback to filename completion for args

    rl_attempted_completion_over = 1;  // prevent default completion
    return rl_completion_matches(text, my_generator);
}

void readline_init() { rl_attempted_completion_function = my_completion; }

char* read_command_line(void) {
    char* line;

    line = readline("$ ");
    if (!line) return NULL;

    if (*line) add_history(line);

    return line;  // caller owns it
}
