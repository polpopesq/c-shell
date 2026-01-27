#ifndef SHELL_H
#define SHELL_H

#ifdef _WIN32
#define PATH_LIST_SEPARATOR ";"
#else
#define PATH_LIST_SEPARATOR ":"
#endif

#include <stddef.h>

#define MAX_REDIR 8
#define MAX_ARGV 16

typedef struct {
    int target_fd;
    enum { TRUNC, APPEND, READ } mode;
    char* filename;
} Redirection;

typedef struct {
    char* argv[MAX_ARGV];
    int argc;
    Redirection redirections[MAX_REDIR];
    int redirc;
} Command;

typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} StringList;

typedef struct {
    Command* cmds;
    size_t count;
} Pipeline;

void list_init(StringList* list, size_t initial_capacity);
void list_append(StringList* list, const char* s);
void free_string_list(StringList* list);

#endif