#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "exec/exec.h"
#include "input/input.h"
#include "parse/parser.h"
#include "util/scanners.h"

int main(void) {
    build_path_cache();
    readline_init();
    char* line;

    while (1) {
        line = read_command_line();
        if (!line) break;

        if (*line) {
            ParsedCommand cmd = parse_command(line);
            execute_command(&cmd);
            free_parsed_command(&cmd);
        }

        free(line);
    }
    return 0;
}
