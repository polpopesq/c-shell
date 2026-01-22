#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

ParsedCommand parse_command(char* line);
void free_parsed_command(ParsedCommand* pc);

#endif
