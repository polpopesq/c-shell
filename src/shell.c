#include "shell.h"

#include <stdlib.h>
#include <string.h>

#define STRINGLIST_INITIAL_CAPACITY 16

// Initialize the list with a starting capacity
void list_init(StringList* list, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = STRINGLIST_INITIAL_CAPACITY;
    list->items = malloc(sizeof(char*) * initial_capacity);
    list->count = 0;
    list->capacity = initial_capacity;
}

// Append a string, reallocating only if needed
void list_append(StringList* list, const char* s) {
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        char** tmp = realloc(list->items, sizeof(char*) * new_capacity);
        if (!tmp) return;  // malloc failed, silently ignore
        list->items = tmp;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = strdup(s);
}

// Free all strings and reset the list
void free_string_list(StringList* list) {
    if (!list->items) return;
    for (size_t i = 0; i < list->count; i++) free(list->items[i]);
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
