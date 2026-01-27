#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <unistd.h>

#include "input.h"
#include "util/scanners.h"

static const char* builtin_candidates[] = {"echo", "cd",      "pwd", "type",
                                           "exit", "history", NULL};

char* builtin_generator(const char* text, int state) {
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

char* path_generator(const char* text, int state) {
    static size_t i;
    size_t len = strlen(text);

    if (state == 0) i = 0;

    const StringList* cache = get_path_cache();

    while (i < cache->count) {
        const char* cand = cache->items[i++];
        if (strncmp(cand, text, len) == 0) return strdup(cand);
    }
    return NULL;
}

static StringList cwd_cache = {0};
static char cwd_path[1024] = {0};

char* cwd_generator(const char* text, int state) {
    static size_t i;
    char tmp[1024];
    getcwd(tmp, sizeof(tmp));

    if (state == 0 || strcmp(tmp, cwd_path) != 0) {
        free_string_list(&cwd_cache);
        cwd_cache = scan_current_directory();
        strcpy(cwd_path, tmp);
        i = 0;
    }

    while (i < cwd_cache.count) {
        const char* cand = cwd_cache.items[i++];
        if (strncmp(cand, text, strlen(text)) == 0) return strdup(cand);
    }

    return NULL;
}

static char** merge_matches(char** a, char** b) {
    if (!a) return b;
    if (!b) return a;

    size_t i, j;
    for (i = 0; a[i]; i++);
    for (j = 0; b[j]; j++);

    a = realloc(a, sizeof(char*) * (i + j + 1));
    memcpy(a + i, b, sizeof(char*) * (j + 1));

    free(b);
    return a;
}

char** custom_shell_completion(const char* text, int start, int end) {
    (void)end;

    if (start != 0) return NULL;  // let readline do filename completion

    rl_attempted_completion_over = 1;

    char** builtins = rl_completion_matches(text, builtin_generator);
    char** path = rl_completion_matches(text, path_generator);
    char** cwd = rl_completion_matches(text, cwd_generator);

    char** result = merge_matches(builtins, path);
    result = merge_matches(result, cwd);

    return result;
}

void readline_init() {
    rl_attempted_completion_function = custom_shell_completion;
}

char* read_command_line(void) {
    char* line;

    line = readline("$ ");
    if (!line) return NULL;

    if (*line) add_history(line);

    return line;  // caller owns it
}
