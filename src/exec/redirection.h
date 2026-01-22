#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "shell.h"

int apply_redirections(const ParsedCommand*, int saved_fds[3]);
void restore_fds(int saved_fds[3]);

#endif
