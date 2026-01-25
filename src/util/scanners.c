#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "scanners.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ds/hashset.h"

static StringList path_cache;

const StringList* get_path_cache(void) { return &path_cache; }

void build_path_cache(void) {
    free_string_list(&path_cache);
    path_cache = scan_path();
}

/* ------------------------------------------------------------ */
/* PATH scanning (WSL-optimized)                                */
/* ------------------------------------------------------------ */

StringList scan_path(void) {
    StringList result;
    list_init(&result, 1024);

    const char* PATH = getenv("PATH");
    if (!PATH) return result;

    char* path = strdup(PATH);
    if (!path) return result;

    char* saveptr = NULL;

    /* Deduplicate PATH directories */
    HashSet seen_dirs;
    hashset_init(&seen_dirs, 64);

    for (char* dir = strtok_r(path, ":", &saveptr); dir;
         dir = strtok_r(NULL, ":", &saveptr)) {
        if (!dir[0]) continue;
        if (!hashset_add(&seen_dirs, dir)) continue;

        /* Optional but HUGE win on WSL */
        /* Skip Windows-mounted paths */
        if (strncmp(dir, "/mnt/", 5) == 0) continue;

        DIR* d = opendir(dir);
        if (!d) continue;

        int dfd = dirfd(d);
        if (dfd == -1) {
            closedir(d);
            continue;
        }

        struct dirent* e;
        while ((e = readdir(d))) {
            if (e->d_name[0] == '.') continue;

            /* Fast reject */
            if (e->d_type == DT_DIR) continue;

            if (e->d_type == DT_REG) {
                /* Fast path: no stat unless needed */
                struct stat st;
                if (fstatat(dfd, e->d_name, &st, 0) == -1) continue;

                if (!(st.st_mode & 0111)) continue;

                list_append(&result, e->d_name);
                continue;
            }

            if (e->d_type == DT_UNKNOWN) {
                /* Slow path: unavoidable syscall */
                struct stat st;
                if (fstatat(dfd, e->d_name, &st, 0) == -1) continue;

                if (!S_ISREG(st.st_mode)) continue;

                if (!(st.st_mode & 0111)) continue;

                list_append(&result, e->d_name);
            }
        }

        closedir(d);
    }

    hashset_free(&seen_dirs);
    free(path);
    return result;
}

/* ------------------------------------------------------------ */
/* Current directory scanning (WSL-optimized)                   */
/* ------------------------------------------------------------ */

StringList scan_current_directory(void) {
    StringList result;
    list_init(&result, 0);

    DIR* d = opendir(".");
    if (!d) return result;

    int dfd = dirfd(d);
    if (dfd == -1) {
        closedir(d);
        return result;
    }

    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        if (e->d_type == DT_DIR) continue;

        if (e->d_type == DT_REG) {
            struct stat st;
            if (fstatat(dfd, e->d_name, &st, 0) == -1) continue;

            if (!(st.st_mode & 0111)) continue;

            list_append(&result, strdup(e->d_name));
            continue;
        }

        if (e->d_type == DT_UNKNOWN) {
            struct stat st;
            if (fstatat(dfd, e->d_name, &st, 0) == -1) continue;

            if (!S_ISREG(st.st_mode)) continue;

            if (!(st.st_mode & 0111)) continue;

            list_append(&result, strdup(e->d_name));
        }
    }

    closedir(d);
    return result;
}
