#ifndef HASHSET_H
#define HASHSET_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char** keys;
    size_t capacity;
    size_t size;
} HashSet;

/* Initialize with a fixed capacity (will round up internally) */
void hashset_init(HashSet* set, size_t initial_capacity);

/* Free all memory owned by the set */
void hashset_free(HashSet* set);

/*
 * Add key to set.
 * Returns true if inserted, false if already present.
 */
bool hashset_add(HashSet* set, const char* key);

#endif
