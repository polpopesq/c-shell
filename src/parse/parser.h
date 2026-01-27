#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

Pipeline parse_pipeline(char* line);
void free_pipeline(Pipeline* pl);

#endif
