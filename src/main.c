#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "builtin/builtin.h"
#include "exec/exec.h"
#include "input/input.h"
#include "parse/parser.h"
#include "util/scanners.h"

int main(void) {
    build_path_cache();
    readline_init();
    char* line;

    initialize_history();

    while (1) {
        line = read_command_line();
        if (!line) break;

        if (*line) {
            Pipeline pipeline = parse_pipeline(line);
            execute_pipeline(&pipeline);
            free_pipeline(&pipeline);
        }

        free(line);
    }

    save_history();
    return 0;
}
