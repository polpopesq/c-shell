#ifndef BUILTIN_H
#define BUILTIN_H

#include "shell.h"  // for ParsedCommand

typedef int (*builtin_func)(const ParsedCommand*);

builtin_func find_builtin(const char* name);

/* builtin declarations */
int exec_cd(const ParsedCommand*);
int exec_pwd(const ParsedCommand*);
int exec_echo(const ParsedCommand*);
int exec_exit(const ParsedCommand*);
int exec_type(const ParsedCommand*);

#endif