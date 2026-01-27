#include "builtin.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    const char* name;
    builtin_func function;
} builtin_entry;

static builtin_entry builtins[] = {
    {"cd", exec_cd},     {"pwd", exec_pwd},   {"echo", exec_echo},
    {"exit", exec_exit}, {"type", exec_type}, {"history", exec_history}};

builtin_func find_builtin(const char* name) {
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(name, builtins[i].name) == 0) return builtins[i].function;
    }
    return NULL;
}
