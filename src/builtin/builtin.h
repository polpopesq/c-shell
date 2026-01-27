#ifndef BUILTIN_H
#define BUILTIN_H

#include "shell.h"  // for ParsedCommand

typedef int (*builtin_func)(const Command*);

builtin_func find_builtin(const char* name);

/* builtin declarations */
int exec_cd(const Command*);
int exec_pwd(const Command*);
int exec_echo(const Command*);
int exec_exit(const Command*);
int exec_type(const Command*);
int exec_history(const Command*);

#endif