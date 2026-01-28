#ifndef SCANNERS_H
#define SCANNERS_H
#include "shell.h"

StringList scan_path(void);
StringList scan_current_directory(void);
void build_path_cache();
void free_path_cache();
const StringList* get_path_cache(void);

#endif