#include <stdlib.h>

#include "exec/exec.h"
#include "input/input.h"
#include "parse/parser.h"

int main(void) {
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
