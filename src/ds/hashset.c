#include "hashset.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR_NUM 7
#define LOAD_FACTOR_DEN 10

/* ------------------------------------------------------------ */
/* Hash function (FNV-1a)                                       */
/* ------------------------------------------------------------ */

static uint64_t hash_str(const char* s) {
    uint64_t h = 1469598103934665603ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

/* ------------------------------------------------------------ */
/* Internal helpers                                             */
/* ------------------------------------------------------------ */

static size_t next_pow2(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

static void hashset_rehash(HashSet* set, size_t new_cap) {
    char** old_keys = set->keys;
    size_t old_cap = set->capacity;

    set->keys = calloc(new_cap, sizeof(char*));
    set->capacity = new_cap;
    set->size = 0;

    for (size_t i = 0; i < old_cap; ++i) {
        if (!old_keys[i]) continue;

        const char* key = old_keys[i];
        uint64_t h = hash_str(key);
        size_t idx = h & (new_cap - 1);

        while (set->keys[idx]) idx = (idx + 1) & (new_cap - 1);

        set->keys[idx] = (char*)key;
        set->size++;
    }

    free(old_keys);
}

/* ------------------------------------------------------------ */
/* Public API                                                   */
/* ------------------------------------------------------------ */

void hashset_init(HashSet* set, size_t initial_capacity) {
    size_t cap = next_pow2(initial_capacity ? initial_capacity : 16);
    set->keys = calloc(cap, sizeof(char*));
    set->capacity = cap;
    set->size = 0;
}

void hashset_free(HashSet* set) {
    if (!set->keys) return;

    for (size_t i = 0; i < set->capacity; ++i) free(set->keys[i]);

    free(set->keys);
    set->keys = NULL;
    set->capacity = 0;
    set->size = 0;
}

bool hashset_add(HashSet* set, const char* key) {
    if ((set->size + 1) * LOAD_FACTOR_DEN > set->capacity * LOAD_FACTOR_NUM) {
        hashset_rehash(set, set->capacity * 2);
    }

    uint64_t h = hash_str(key);
    size_t idx = h & (set->capacity - 1);

    while (set->keys[idx]) {
        if (strcmp(set->keys[idx], key) == 0) return false;  // already present
        idx = (idx + 1) & (set->capacity - 1);
    }

    set->keys[idx] = strdup(key);
    set->size++;
    return true;
}
